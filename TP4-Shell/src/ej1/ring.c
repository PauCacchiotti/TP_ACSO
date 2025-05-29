#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv){	
	int start, pid, n;
	int buffer[1]; 

	if (argc != 4){
		printf("Uso: anillo <n> <c> <s> \n");
		exit(0);
	}

	n = atoi(argv[1]);
	buffer[0] = atoi(argv[2]);
	start = atoi(argv[3]);

	if (start < 0 || start >= n) {
    	fprintf(stderr, "Error: índice de inicio fuera de rango\n");
    	exit(1);
	}

	printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, buffer[0], start);

	int padre_pipe[2];
	if (pipe(padre_pipe) == -1) {
		perror("pipe padre");
		exit(1);
	}

	int pipes[n][2];
	for (int i = 0; i < n; i++) {
		if (pipe(pipes[i]) == -1) {
			perror("pipe");
			exit(1);
		}
	}

	for (int i = 0; i < n; i++) {
		pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(1);
		} else if (pid == 0) {
			for (int j = 0; j < n; j++) {
				if (j != i) close(pipes[j][0]); 
				if (j != (i + 1) % n) close(pipes[j][1]); 
			}
			close(padre_pipe[0]); 
			if (i != (start + n - 1) % n)
				close(padre_pipe[1]); 

			int val;
			read(pipes[i][0], &val, sizeof(int));
			val++;
			if (i == (start + n - 1) % n) {
				write(padre_pipe[1], &val, sizeof(int));
			} else {
				write(pipes[(i + 1) % n][1], &val, sizeof(int));
			}
			exit(0);
		}
	}

	for (int i = 0; i < n; i++) {
		if (i != start) close(pipes[i][1]); 
		close(pipes[i][0]); 
	}
	close(padre_pipe[1]); 

	write(pipes[start][1], &buffer[0], sizeof(int));
	close(pipes[start][1]);

	read(padre_pipe[0], &buffer[0], sizeof(int));
	close(padre_pipe[0]);

	for (int i = 0; i < n; i++) wait(NULL);

	printf("El valor final recibido por el padre es: %d\n", buffer[0]);
	return 0;
}