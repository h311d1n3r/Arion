; test already contain builded shellcode
; build : nasm -f elf64 hello_world_x86_64.s -o hello_x86_64.o

section .text
    global _start

_start:
    sub rsp, 16
    mov dword [rsp], 0x48000000     ; 'H'
    mov dword [rsp+4], 0x6F6C6C65   ; 'ello'
    mov dword [rsp+8], 0x726F5720   ; ' Wor'
    mov dword [rsp+12], 0x0A21646C   ; ' ld!\n'

    mov rax, 1          ; syscall number for sys_write
    mov rdi, 1          ; file descriptor (stdout)
    mov rsi, rsp        ; pointer to message (stack)
    add rsi, 3          ; skil pad
    mov rdx, 14         ; message length
    syscall             ; make syscall

    mov rax, 60         ; syscall number for sys_exit
    xor rdi, rdi        ; exit code 0
    syscall