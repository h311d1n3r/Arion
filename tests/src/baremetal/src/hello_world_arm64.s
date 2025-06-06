.text
.global _start

// Compile with:
// aarch64-linux-gnu-as hello_world_arm64.s -o hello64.o

_start:
    sub sp, sp, #16         // allocate stack space
    mov x0, sp              // x0 = pointer to string
    
    // Load first 8 bytes: " World!\n"
    ldr x1, =0x0a21646c726f5720
    str x1, [x0, #8]

    // Load second 8 bytes: "Hello" + padding
    ldr x1, =0x6f6c6c6548000000
    str x1, [x0]
    str x1, [x0]

    mov x1, x0               // x1 = string pointer
    add x1, x1, #3           // skip pad

    mov x2, #14              // length
    mov x0, #1               // fd = 1 (stdout)
    mov x8, #64              // syscall number for write
    svc #0

    mov x0, #0               // exit code
    mov x8, #93              // syscall number for exit
    svc #0
