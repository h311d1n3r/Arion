#include <arion/arion.hpp>
#include <arion/platforms/linux/archs/lnx_arch_x86.hpp>

using namespace arion;

std::map<arion::REG, arion::RVAL> ArchManagerLinuxX86::prstatus_to_regs(std::vector<arion::BYTE> prstatus)
{
    std::unique_ptr<arion_lnx_x86::elf_prstatus> prstatus_struct = std::make_unique<arion_lnx_x86::elf_prstatus>();
    memcpy(prstatus_struct.get(), prstatus.data(), prstatus.size());
    struct arion_lnx_x86::user_regs_struct *regs_struct =
        (struct arion_lnx_x86::user_regs_struct *)&prstatus_struct->pr_reg;
    std::map<REG, RVAL> regs;
    for (off_t reg_i = 0; reg_i < arion_lnx_x86::uc_user_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_x86::uc_user_regs.at(reg_i);
        if (reg == UC_X86_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r32 = ((RVAL64 *)regs_struct)[reg_i];
        regs[reg] = rval;
    }
    return regs;
}
