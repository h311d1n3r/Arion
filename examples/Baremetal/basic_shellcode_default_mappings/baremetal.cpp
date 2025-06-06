#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/baremetal.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>
#include <filesystem>

using namespace arion;

/* Simple Hello World!  */

// x64 (64-bit)
/*
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
*/
unsigned char shellcode_x8664[] = {
    0x48, 0x83, 0xec, 0x10,0xc7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x48, 
    0xc7, 0x44, 0x24, 0x04, 0x65, 0x6c, 0x6c, 0x6f,0xc7, 0x44, 0x24,
    0x08, 0x20, 0x57, 0x6f,0x72, 0xc7, 0x44, 0x24, 0x0c, 0x6c, 0x64,
    0x21, 0x0a, 0xb8, 0x01, 0x00, 0x00, 0x00,0xbf, 0x01, 0x00, 0x00,
    0x00, 0x48, 0x89, 0xe6, 0x48, 0x83, 0xc6, 0x03,0xba, 0x0e, 0x00,
    0x00, 0x00, 0x0f, 0x05, 0xb8, 0x3c, 0x00, 0x00, 0x00,0x48, 0x31, 
    0xff,0x0f, 0x05                          
};

// x86 (32-bit)
/*
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
*/
unsigned char shellcode_x86[] = {
    0x68,0x6c,0x64,0x21,0x0a,0x68,0x20,0x57,0x6f,0x72, 
    0x68,0x65,0x6c,0x6c,0x6f,0x68,0x00,0x00,0x00,0x48,
    0xb8,0x04,0x00,0x00,0x00,0xbb,0x01,0x00,0x00,0x00,
    0x89,0xe1,0x83,0xc1,0x03,0xba,0x0e,0x00,0x00,0x00,
    0xcd,0x80,0xb8,0x01,0x00,0x00,0x00,0x31,0xdb,0xcd,
    0x80,
};

// arm (32bit)
/*
.text
.global _start

@compile with arm-linux-gnueabi-as hello_world_arm.s -o hello_arm.o
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
*/
unsigned char shellcode_arm[] = {
    0x10,0xd0,0x4d,0xe2,0x0d,0x00,0xa0,0xe1,0x3c,0x10,0x9f,0xe5,
    0x0c,0x10,0x80,0xe5,0x38,0x10,0x9f,0xe5,0x08,0x10,0x80,0xe5,
    0x34,0x10,0x9f,0xe5,0x04,0x10,0x80,0xe5,0x12,0x13,0xa0,0xe3,
    0x00,0x10,0x80,0xe5,0x00,0x10,0xa0,0xe1,0x03,0x10,0x81,0xe2,
    0x0e,0x20,0xa0,0xe3,0x04,0x70,0xa0,0xe3,0x01,0x00,0xa0,0xe3,
    0x00,0x00,0x00,0xef,0x00,0x00,0xa0,0xe3,0x01,0x70,0xa0,0xe3,
    0x00,0x00,0x00,0xef,0x6c,0x64,0x21,0x0a,0x20,0x57,0x6f,0x72,
    0x65,0x6c,0x6c,0x6f
};

/*
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
*/
unsigned char shellcode_arm64[] = {
    0xff,0x43,0x00,0xd1,0xe0,0x03,0x00,0x91,0xc1,0x01,0x00,0x58,
    0x01,0x04,0x00,0xf9,0xc1,0x01,0x00,0x58,0x01,0x00,0x00,0xf9,
    0x01,0x00,0x00,0xf9,0xe1,0x03,0x00,0xaa,0x21,0x0c,0x00,0x91,
    0xc2,0x01,0x80,0xd2,0x20,0x00,0x80,0xd2,0x08,0x08,0x80,0xd2,
    0x01,0x00,0x00,0xd4,0x00,0x00,0x80,0xd2,0xa8,0x0b,0x80,0xd2,
    0x01,0x00,0x00,0xd4,0x20,0x57,0x6f,0x72,0x6c,0x64,0x21,0x0a,
    0x00,0x00,0x00,0x48,0x65,0x6c,0x6c,0x6f
};

using namespace arion;

void instr_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    std::vector<BYTE> read_data = arion->mem->read(addr, sz);
    cs_insn *insn;
    size_t count = cs_disasm(*arion->abi->curr_cs(), (const uint8_t *)read_data.data(), sz, addr, 0, &insn);
    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            std::cout << std::hex << insn[i].address << ":\t";
            for (size_t j = 0; j < insn[i].size; j++)
                std::cout << std::hex << std::setw(2) << std::setfill('0') << +insn[i].bytes[j] << " ";
            if (insn[i].size < 8)
                std::cout << std::string((8 - insn[i].size) * 3, ' ');
            std::cout << "\t" << insn[i].mnemonic << "\t" << insn[i].op_str << std::endl;
        }

        cs_free(insn, count);
    }
    else
    {
        std::cerr << "Failed to disassemble code at 0x" << std::hex << addr << std::endl;
    }
}

int main()
{
    std::map<CPU_ARCH, std::pair<unsigned char*, size_t>> shellcodes = {
        {CPU_ARCH::X86_ARCH,     {shellcode_x86,     sizeof(shellcode_x86)}},
        {CPU_ARCH::X8664_ARCH,   {shellcode_x8664,  sizeof(shellcode_x8664)}},
        {CPU_ARCH::ARM_ARCH,     {shellcode_arm,     sizeof(shellcode_arm)}},
        {CPU_ARCH::ARM64_ARCH,   {shellcode_arm64,   sizeof(shellcode_arm64)}}
    };

    for (const auto& [arch, shell_data] : shellcodes)
    {
        std::unique_ptr<Config> config = std::make_unique<Config>();
        std::unique_ptr<Baremetal> baremetal = std::make_unique<Baremetal>();
        baremetal->arch = arch;
        auto coderaw = baremetal->coderaw;
        coderaw->insert(coderaw->end(), shell_data.first, shell_data.first + shell_data.second);

        config->set_field<ARION_LOG_LEVEL>("log_lvl", ARION_LOG_LEVEL::DEBUG);

        std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
        std::shared_ptr<Arion> arion = 
            Arion::new_instance(std::move(baremetal), "/", {}, std::filesystem::current_path(), std::move(config));

        arion_group->add_arion_instance(arion);

        std::cout << arion->mem->mappings_str() << std::endl;

        arion->hooks->hook_code(instr_hook);
        arion_group->run();
    }

    return 0;
}

