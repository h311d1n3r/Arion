.text
.global _start

@shellcodes already saved in examples/test 
@compile with ../../../../rootfs/arm/toolchains/bin/arm-linux-gnueabi-as hello_world_arm.s -o hello_arm.o
_start:
    sub sp, sp, #16         @stack setup

    @ Push "Hello, World!\n" in reverse (little-endian)
    mov r0, sp              @ r0 = pointer to string
    ldr r1, =0x0a21646c     @ "ld!\n"
    str r1, [r0, #12]
    ldr r1, =0x726f5720     @ " Wor"
    str r1, [r0, #8]
    ldr r1, =0x6f6c6c65     @ "ello"
    str r1, [r0, #4]
    ldr r1, =0x48000000     @ "H"
    str r1, [r0]

    mov r1, r0              @ message pointer
    add r1, #3              @ skip null bytes
    mov r2, #14             @ message length
    mov r7, #4              @ syscall: write
    mov r0, #1              @ stdout (fd = 1)
    svc 0

    mov r0, #0              @ exit code
    mov r7, #1              @ syscall: exit
    svc 0


