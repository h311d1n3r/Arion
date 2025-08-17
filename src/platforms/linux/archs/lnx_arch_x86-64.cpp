#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/archs/lnx_arch_x86-64.hpp>

using namespace arion;

std::map<arion::REG, arion::RVAL> ArchManagerLinuxX8664::prstatus_to_regs(std::vector<arion::BYTE> prstatus)
{
    std::unique_ptr<arion_lnx_x86_64::elf_prstatus> prstatus_struct =
        std::make_unique<arion_lnx_x86_64::elf_prstatus>();
    memcpy(prstatus_struct.get(), prstatus.data(), prstatus.size());
    struct arion_lnx_x86_64::user_regs_struct *regs_struct =
        (struct arion_lnx_x86_64::user_regs_struct *)&prstatus_struct->pr_reg;
    std::map<REG, RVAL> regs;
    for (off_t reg_i = 0; reg_i < arion_lnx_x86_64::uc_user_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_x86_64::uc_user_regs.at(reg_i);
        if (reg == UC_X86_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r64 = ((RVAL64 *)regs_struct)[reg_i];
        regs[reg] = rval;
    }
    return regs;
}

std::map<arion::REG, arion::RVAL> ArchManagerLinuxX8664::fpregset_to_regs(std::vector<arion::BYTE> fpregset)
{
    std::unique_ptr<arion_lnx_x86_64::elf_fpregset_t> fpregset_struct =
        std::make_unique<arion_lnx_x86_64::elf_fpregset_t>();
    memcpy(fpregset_struct.get(), fpregset.data(), fpregset.size());
    std::map<REG, RVAL> regs;

    for (off_t reg_i = 0; reg_i < arion_lnx_x86_64::uc_st_space_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_x86_64::uc_st_space_regs.at(reg_i);
        if (reg == UC_X86_REG_INVALID)
            continue;
        RVAL rval{};
        uint32_t v1 = ((RVAL32 *)&fpregset_struct->st_space)[reg_i * 4],
                 v2 = ((RVAL32 *)&fpregset_struct->st_space)[reg_i * 4 + 1],
                 v3 = ((RVAL32 *)&fpregset_struct->st_space)[reg_i * 4 + 2],
                 v4 = ((RVAL32 *)&fpregset_struct->st_space)[reg_i * 4 + 3];
        memcpy(rval.r128.data(), &v1, sizeof(v1));
        memcpy(rval.r128.data() + 4, &v2, sizeof(v2));
        memcpy(rval.r128.data() + 8, &v3, sizeof(v3));
        memcpy(rval.r128.data() + 12, &v4, sizeof(v4));
        regs[reg] = rval;
    }

    for (off_t reg_i = 0; reg_i < arion_lnx_x86_64::uc_xmm_space_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_x86_64::uc_xmm_space_regs.at(reg_i);
        if (reg == UC_X86_REG_INVALID)
            continue;
        RVAL rval{};
        uint32_t v1 = ((RVAL32 *)&fpregset_struct->xmm_space)[reg_i * 4],
                 v2 = ((RVAL32 *)&fpregset_struct->xmm_space)[reg_i * 4 + 1],
                 v3 = ((RVAL32 *)&fpregset_struct->xmm_space)[reg_i * 4 + 2],
                 v4 = ((RVAL32 *)&fpregset_struct->xmm_space)[reg_i * 4 + 3];
        memcpy(rval.r128.data(), &v1, sizeof(v1));
        memcpy(rval.r128.data() + 4, &v2, sizeof(v2));
        memcpy(rval.r128.data() + 8, &v3, sizeof(v3));
        memcpy(rval.r128.data() + 12, &v4, sizeof(v4));
        regs[reg] = rval;
    }

    return regs;
}
