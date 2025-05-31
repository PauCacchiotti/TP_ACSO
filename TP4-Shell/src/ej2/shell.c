#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>
#include <wordexp.h>  

#define MAX_COMMANDS 200

void parse_args(char *cmd, char **argv) {
    int i = 0;
    while (*cmd) {
        while (isspace(*cmd)) cmd++; 
        if (*cmd == '"') {
            cmd++; 
            argv[i++] = cmd;
            while (*cmd && *cmd != '"') cmd++;
            if (*cmd) *cmd++ = '\0'; 
        } else if (*cmd) {
            argv[i++] = cmd;
            while (*cmd && !isspace(*cmd)) cmd++;
            if (*cmd) *cmd++ = '\0';
        }
    }
    argv[i] = NULL;
}

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];

    while (1) {
        printf("Shell> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) break;
        command[strcspn(command, "\n")] = '\0';

        int command_count = 0;
        char *token = strtok(command, "|");
        while (token != NULL) {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        int prev_pipe[2] = {-1, -1};

        for (int i = 0; i < command_count; i++) {
            int curr_pipe[2];
            if (i < command_count - 1 && pipe(curr_pipe) == -1) {
                perror("pipe");
                exit(1);
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            }

            if (pid == 0) {
                if (i > 0) {
                    dup2(prev_pipe[0], STDIN_FILENO);
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }

                if (i < command_count - 1) {
                    dup2(curr_pipe[1], STDOUT_FILENO);
                    close(curr_pipe[0]);
                    close(curr_pipe[1]);
                }

                wordexp_t p;
                if (wordexp(commands[i], &p, 0) != 0) {
                    fprintf(stderr, "Error al parsear argumentos\n");
                    exit(1);
                }

                execvp(p.we_wordv[0], p.we_wordv);
                perror("execvp");
                wordfree(&p);
                exit(1);

            }

            if (i > 0) {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }

            if (i < command_count - 1) {
                prev_pipe[0] = curr_pipe[0];
                prev_pipe[1] = curr_pipe[1];
            }
        }

        for (int i = 0; i < command_count; i++) {
            wait(NULL);
        }
    }

    return 0;
}