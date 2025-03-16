#include <arion/archs/abi_x86.hpp>
#include <arion/arion.hpp>

using namespace arion;

ks_engine *AbiManagerX86::curr_ks()
{
    return this->ks.at(0);
}

csh *AbiManagerX86::curr_cs()
{
    return this->cs.at(0);
}

std::array<arion::BYTE, VSYSCALL_ENTRY_SZ> AbiManagerX86::gen_vsyscall_entry(uint64_t syscall_no)
{
    return std::array<arion::BYTE, VSYSCALL_ENTRY_SZ>();
}

void AbiManagerX86::int_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (intno == 0x80)
        arion->syscalls->process_syscall(arion);
}

void AbiManagerX86::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->hooks->hook_intr(AbiManagerX86::int_hook);
}
