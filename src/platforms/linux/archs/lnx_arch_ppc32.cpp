#include <arion/platforms/linux/archs/lnx_arch_ppc32.hpp>

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>

using namespace arion;
using namespace arion_lnx_ppc32;

std::map<arion::REG, arion::RVAL> ArchManagerLinuxPPC32::prstatus_to_regs(std::vector<arion::BYTE> prstatus)
{
    std::unique_ptr<elf_prstatus> prstatus_struct = std::make_unique<elf_prstatus>();
    size_t copy_sz = std::min(prstatus.size(), sizeof(elf_prstatus));
    std::memcpy(prstatus_struct.get(), prstatus.data(), copy_sz);

    std::map<REG, RVAL> regs;
    for (size_t reg_i = 0; reg_i < uc_pt_regs.size() && reg_i < ELF_NGREG; reg_i++)
    {
        REG reg = uc_pt_regs.at(reg_i);
        if (reg == UC_PPC_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r32 = static_cast<RVAL32>(prstatus_struct->pr_reg[reg_i]);
        regs[reg] = rval;
    }

    return regs;
}

std::map<arion::REG, arion::RVAL> ArchManagerLinuxPPC32::fpregset_to_regs(std::vector<arion::BYTE> fpregset)
{
    std::unique_ptr<elf_fpregset_t> fpregset_struct = std::make_unique<elf_fpregset_t>();
    size_t copy_sz = std::min(fpregset.size(), sizeof(elf_fpregset_t));
    std::memcpy(fpregset_struct.get(), fpregset.data(), copy_sz);

    std::map<REG, RVAL> regs;
    for (size_t reg_i = 0; reg_i < uc_fp_regs.size(); reg_i++)
    {
        REG reg = uc_fp_regs.at(reg_i);
        if (reg == UC_PPC_REG_INVALID)
            continue;
        uint64_t raw = 0;
        if (reg_i < 32)
            std::memcpy(&raw, &fpregset_struct->fpregs[reg_i], sizeof(raw));
        else
            std::memcpy(&raw, &fpregset_struct->fpscr, sizeof(raw));
        RVAL rval{};
        rval.r64 = raw;
        regs[reg] = rval;
    }

    return regs;
}
