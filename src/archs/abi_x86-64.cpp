#include <arion/archs/abi_x86-64.hpp>
#include <arion/arion.hpp>

using namespace arion;

ks_engine *AbiManagerX8664::curr_ks()
{
    return this->ks.at(0);
}

csh *AbiManagerX8664::curr_cs()
{
    return this->cs.at(0);
}

std::array<BYTE, VSYSCALL_ENTRY_SZ> AbiManagerX8664::gen_vsyscall_entry(uint64_t syscall_no)
{
    const char vsyscall_asm_str[] = "mov rax, 0x%" PRIx64 "; syscall; ret";
    size_t vsyscall_asm_sz = strlen(vsyscall_asm_str) + 16;
    char *vsyscall_asm = (char *)malloc(vsyscall_asm_sz);
    snprintf(vsyscall_asm, vsyscall_asm_sz, vsyscall_asm_str, syscall_no);
    unsigned char *asm_buf;
    size_t asm_sz, asm_cnt;
    ks_err ks_asm_err = (ks_err)ks_asm(this->ks.at(0), vsyscall_asm, 0, &asm_buf, &asm_sz, &asm_cnt);
    if (ks_asm_err != KS_ERR_OK)
        throw KeystoneAsmException(ks_asm_err);
    std::array<BYTE, VSYSCALL_ENTRY_SZ> vsyscall_entry;
    std::copy(asm_buf, asm_buf + asm_sz, vsyscall_entry.begin());
    std::fill(vsyscall_entry.begin() + asm_sz, vsyscall_entry.end(), 0xCC); // INT3
    free(asm_buf);
    free(vsyscall_asm);
    return vsyscall_entry;
}

void AbiManagerX8664::syscall_hook(std::shared_ptr<Arion> arion, void *user_data)
{
    arion->syscalls->process_syscall(arion);
}

void AbiManagerX8664::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->hooks->hook_insn(AbiManagerX8664::syscall_hook, UC_X86_INS_SYSCALL);
}
