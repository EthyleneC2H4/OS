global my_print
    SECTION .text
my_print:
    push ebp
    mov ebp, esp
    mov edx, [ebp+12]
    mov ecx, [ebp+8]
    mov ebx, 1;stdout
    mov eax, 4;sys_write
    int 80h
    pop ebp
    ret