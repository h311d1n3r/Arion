#include <arion/archs/arch_ppc32.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>

using namespace arion;
using namespace arion_ppc32;
using namespace arion_exception;

void ArchManagerPPC32::code_hook(std::shared_ptr<Arion> arion, arion::ADDR address, uint32_t size, void *user_data)
{
    (void)user_data;
    if (size != sizeof(uint32_t))
        return;

    std::vector<BYTE> insn = arion->mem->read(address, size);
    if (insn.size() != sizeof(uint32_t))
        return;

    uint32_t opcode = (static_cast<uint32_t>(insn[0]) << 24) | (static_cast<uint32_t>(insn[1]) << 16) |
                      (static_cast<uint32_t>(insn[2]) << 8) | static_cast<uint32_t>(insn[3]);
    if (opcode == SC_OPCODE)
        arion->syscalls->process_syscall(arion);
}

void ArchManagerPPC32::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->hooks->hook_code(ArchManagerPPC32::code_hook);
}

ks_engine *ArchManagerPPC32::curr_ks()
{
    if (this->ks.empty())
        return nullptr;
    return this->ks.at(0);
}

csh *ArchManagerPPC32::curr_cs()
{
    if (this->cs.empty())
        return nullptr;
    return this->cs.at(0);
}

ADDR ArchManagerPPC32::dump_tls()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    return arion->arch->read_reg<RVAL32>(UC_PPC_REG_13);
}

void ArchManagerPPC32::load_tls(ADDR new_tls)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->arch->write_reg<RVAL32>(UC_PPC_REG_13, static_cast<RVAL32>(new_tls));
}
