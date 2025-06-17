; test already contain builded shellcode
; build : nasm -f elf32 hello_x86.asm -o hello_x86.o

section .text
    global _start

_start:
    push 0x0A21646C        ; 'l','d','!','\n'
    push 0x726F5720        ; ' ', 'W', 'o', 'r'
    push 0x6F6C6C65        ; 'e', 'l', 'l', 'o'
    push 0x48000000        ; 'H'

    mov eax, 4             ; sys_write
    mov ebx, 1             ; stdout
    mov ecx, esp           ; pointer to message
    add ecx, 3             ; skip padding
    mov edx, 14            ; message length
    int 0x80

    mov eax, 1             ; sys_exit
    xor ebx, ebx           ; status = 0
    int 0x80
