#include <arion/arion.hpp>
#include <arion/platforms/linux/archs/lnx_arch_arm.hpp>

using namespace arion;

uint64_t ArchManagerLinuxARM::pack_fp_reg(const struct arion_lnx_arm::fp_reg *r)
{
    uint64_t v = 0;
    v |= (uint64_t)r->sign1 << 63;
    v |= (uint64_t)r->sign2 << 62;
    v |= (uint64_t)r->exponent << 48;
    v |= (uint64_t)r->j << 47;
    v |= (uint64_t)r->mantissa1 << 16;
    v |= (uint64_t)r->mantissa0;
    return v;
}

std::map<arion::REG, arion::RVAL> ArchManagerLinuxARM::prstatus_to_regs(std::vector<arion::BYTE> prstatus)
{
    std::unique_ptr<arion_lnx_arm::elf_prstatus> prstatus_struct = std::make_unique<arion_lnx_arm::elf_prstatus>();
    memcpy(prstatus_struct.get(), prstatus.data(), prstatus.size());
    struct arion_lnx_arm::pt_regs *regs_struct = (struct arion_lnx_arm::pt_regs *)&prstatus_struct->pr_reg;
    std::map<REG, RVAL> regs;
    for (off_t reg_i = 0; reg_i < arion_lnx_arm::uc_pt_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_arm::uc_pt_regs.at(reg_i);
        if (reg == UC_ARM_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r32 = ((RVAL32 *)regs_struct)[reg_i];
        regs[reg] = rval;
    }
    return regs;
}

std::map<arion::REG, arion::RVAL> ArchManagerLinuxARM::fpregset_to_regs(std::vector<arion::BYTE> fpregset)
{
    std::unique_ptr<arion_lnx_arm::elf_fpregset_t> fpregset_struct = std::make_unique<arion_lnx_arm::elf_fpregset_t>();
    memcpy(fpregset_struct.get(), fpregset.data(), fpregset.size());
    std::map<REG, RVAL> regs;

    for (off_t reg_i = 0; reg_i < arion_lnx_arm::uc_fp_regs.size(); reg_i++)
    {
        REG reg = arion_lnx_arm::uc_fp_regs.at(reg_i);
        if (reg == UC_ARM_REG_INVALID)
            continue;
        RVAL rval{};
        rval.r64 = ((RVAL64 *)&fpregset_struct->fpregs)[reg_i];
        regs[reg] = rval;
    }

    return regs;
}
