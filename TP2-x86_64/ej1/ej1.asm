%define NULL 0
%define TRUE 1
%define FALSE 0

section .data
section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern str_concat
extern strlen
extern strcpy

; ------------------------------
; string_proc_list_create_asm
; ------------------------------
string_proc_list_create_asm:
    push rbp
    mov rbp, rsp
    mov rdi, 16
    call malloc
    test rax, rax
    jz .done
    mov qword [rax], 0        ; first
    mov qword [rax + 8], 0    ; last
.done:
    pop rbp
    ret

; ------------------------------
; string_proc_node_create_asm
; ------------------------------
string_proc_node_create_asm:
    test rsi, rsi
    jz .return_null
    push rdi
    mov rdi, 32
    call malloc
    pop rdi
    test rax, rax
    jz .return_null
    mov qword [rax + 0], 0       ; next
    mov qword [rax + 8], 0       ; previous
    mov byte  [rax + 16], dil    ; type
    mov qword [rax + 24], rsi    ; hash
.done:
    ret
.return_null:
    xor rax, rax
    jmp .done

; ------------------------------
; string_proc_list_add_node_asm
; ------------------------------
string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    test rdi, rdi
    jz .epilogue
    test rdx, rdx
    jz .epilogue

    mov rbx, rdi        ; list
    mov r13b, sil       ; type
    mov r14, rdx        ; hash

    movzx rdi, r13b
    mov rsi, r14
    call string_proc_node_create_asm
    test rax, rax
    jz .epilogue
    mov r12, rax        ; new_node

    cmp qword [rbx], 0
    jne .append

    ; Lista vacía
    mov [rbx], r12
    mov [rbx + 8], r12
    jmp .epilogue

.append:
    mov rcx, [rbx + 8]      ; last
    mov [rcx], r12          ; last->next = new_node
    mov [r12 + 8], rcx      ; new_node->previous = last
    mov [rbx + 8], r12      ; list->last = new_node

.epilogue:
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret

; ------------------------------
; string_proc_list_concat_asm
; ------------------------------
string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    push rbx

    test rdi, rdi
    jz .return_null
    test rdx, rdx
    jz .return_null

    mov r12, rdi        ; list
    mov r13b, sil       ; type
    mov r14, rdx        ; hash

    mov rdi, r14
    call strlen
    mov r15, rax
    inc r15
    mov rdi, r15
    call malloc
    test rax, rax
    jz .return_null
    mov r15, rax

    mov rdi, r15
    mov rsi, r14
    call strcpy

    mov rbx, [r12]
.loop:
    test rbx, rbx
    jz .done

    cmp byte [rbx + 16], r13b
    jne .next

    mov rsi, [rbx + 24]
    test rsi, rsi
    jz .next

    mov rdi, r15
    call str_concat
    test rax, rax
    jz .free_and_null
    mov rdi, r15
    mov r15, rax
    call free

.next:
    mov rbx, [rbx]
    jmp .loop

.free_and_null:
    mov rdi, r15
    call free
    xor r15, r15
    jmp .done

.return_null:
    xor r15, r15

.done:
    mov rax, r15
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    leave
    ret
