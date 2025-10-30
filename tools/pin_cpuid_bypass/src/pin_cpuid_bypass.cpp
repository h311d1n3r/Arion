#include <algorithm>
#include <arion/archs/arch_x86-64.hpp>
#include <arion/common/code_tracer.hpp>
#include <arion/crypto/md5.hpp>
#include <arion/utils/convert_utils.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <pin.H>
#include <random>
#include <string>

const std::string VERSION = "1.0-alpha";
const std::string TESTED_PIN_VERSION = "3.31";

int saved_stdin;
int saved_stdout;
int saved_stderr;

uint64_t leaf_val = 0;

VOID CPUIDHookBefore(CONTEXT *ctx)
{
    std::cout << "[CPUID BYPASS] Hooked CPUID entry." << std::endl;
    leaf_val = PIN_GetContextReg(ctx, REG_RAX);
}

VOID CPUIDHookAfter(CONTEXT *ctx)
{
    std::cout << "[CPUID BYPASS] Hooked CPUID exit." << std::endl;
    uint64_t rbx = PIN_GetContextReg(ctx, REG_RBX);
    uint64_t rcx = PIN_GetContextReg(ctx, REG_RCX);
    uint64_t rdx = PIN_GetContextReg(ctx, REG_RDX);

    uint64_t rbx_mask = 0xFFFFFFFFFFFFFFFF;
    uint64_t rcx_mask = 0xFFFFFFFFFFFFFFFF;
    uint64_t rdx_mask = 0xFFFFFFFFFFFFFFFF;

    switch (leaf_val)
    {
    case 1: {
        rcx_mask ^= 1;         // REMOVE SSE3 SUPPORT
        rcx_mask ^= (1 << 9);  // SSSE3 SUPPORT
        rcx_mask ^= (1 << 19); // SSE4.1 SUPPORT
        rcx_mask ^= (1 << 20); // SSE4.2 SUPPORT
        break;
    }
    case 7: {
        rbx_mask ^= (1 << 5); // REMOVE AVX2 SUPPORT
        rbx_mask ^= (1 << 9); // REMOVE ERMS SUPPORT
        break;
    }
    default:
        break;
    }

    PIN_SetContextReg(ctx, REG_RBX, rbx & rbx_mask);
    PIN_SetContextReg(ctx, REG_RCX, rcx & rcx_mask);
    PIN_SetContextReg(ctx, REG_RDX, rdx & rdx_mask);
    PIN_ExecuteAt(ctx);
}

VOID Cpuid(INS ins, VOID *v)
{
    if (INS_Opcode(ins) == XED_ICLASS_CPUID)
    {
        // Execute without some extensions (AVX2, SSSE3...) because they are not yet supported by QEMU
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CPUIDHookBefore, IARG_CONTEXT, IARG_END);
        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)CPUIDHookAfter, IARG_CONTEXT, IARG_END);
    }
}

VOID Fini(INT32 code, VOID *v)
{
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    std::cout << "Done." << std::endl;
}

std::string GetFormattedVersion(uint8_t len)
{
    std::string version = PIN_Version().substr(9);
    std::string fmt_version = "";

    for (char c : version)
    {
        if (c == '-')
            break;
        fmt_version += c;
    }

    for (uint8_t i = fmt_version.length(); i < len; i++)
        fmt_version += " ";

    return fmt_version;
}

VOID PrintBanner()
{
    std::cout << " ------- PIN CPUID BYPASS -------" << std::endl;
    std::cout << "| Version: " << VERSION << "             |" << std::endl;
    std::cout << "| Tested PIN version: v" << TESTED_PIN_VERSION << "      |" << std::endl;
    std::cout << "| Current PIN version: v" << GetFormattedVersion(9) << "|" << std::endl;
    std::cout << "| Project: Arion                 |" << std::endl;
    std::cout << " --------------------------------" << std::endl;
    std::cout << std::endl;
}

int ARION_EXPORT main(int argc, char *argv[])
{
    PrintBanner();

    saved_stdin = dup(STDIN_FILENO);
    saved_stdout = dup(STDOUT_FILENO);
    saved_stderr = dup(STDERR_FILENO);

    std::cout << "Executing target..." << std::endl;
    PIN_Init(argc, argv);
    INS_AddInstrumentFunction(Cpuid, NULL);
    PIN_AddFiniFunction(Fini, NULL);
    PIN_StartProgram();
    return 0;
}
