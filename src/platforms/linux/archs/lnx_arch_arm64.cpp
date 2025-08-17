#include <arion/arion.hpp>
#include <arion/platforms/linux/archs/lnx_arch_arm64.hpp>

using namespace arion;

std::map<arion::REG, arion::RVAL> ArchManagerLinuxARM64::prstatus_to_regs(std::vector<arion::BYTE> prstatus)
{
    std::unique_ptr<arion_lnx_arm64::elf_prstatus> prstatus_struct = std::make_unique<arion_lnx_arm64::elf_prstatus>();
    memcpy(prstatus_struct.get(), prstatus.data(), prstatus.size());
    struct arion_lnx_arm64::user_pt_regs *regs_struct =
        (struct arion_lnx_arm64::user_pt_regs *)&prstatus_struct->pr_reg;
    std::map<REG, RVAL> regs;
    for (off_t reg_i = 0; reg_i < arion_lnx_arm64::uc_user_pt_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_arm64::uc_user_pt_regs.at(reg_i);
        if (reg == UC_ARM64_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r64 = ((RVAL64 *)regs_struct)[reg_i];
        regs[reg] = rval;
    }
    return regs;
}

std::map<arion::REG, arion::RVAL> ArchManagerLinuxARM64::fpregset_to_regs(std::vector<arion::BYTE> fpregset)
{
    std::unique_ptr<arion_lnx_arm64::elf_fpregset_t> fpregset_struct =
        std::make_unique<arion_lnx_arm64::elf_fpregset_t>();
    memcpy(fpregset_struct.get(), fpregset.data(), fpregset.size());
    std::map<REG, RVAL> regs;

    for (off_t reg_i = 0; reg_i < arion_lnx_arm64::uc_fpsimd_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_arm64::uc_fpsimd_regs.at(reg_i);
        if (reg == UC_ARM64_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r128 = ((RVAL128 *)&fpregset_struct->vregs)[reg_i];
        regs[reg] = rval;
    }

    return regs;
}
