#ifndef ARION_LNX_ARCH_ARM_HPP
#define ARION_LNX_ARCH_ARM_HPP

#include <arion/archs/arch_arm.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_arm
{

struct pt_regs
{
    unsigned long uregs[18];
};

inline std::vector<arion::REG> uc_pt_regs = {
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4,   UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10,  UC_ARM_REG_FP,
    UC_ARM_REG_IP, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_CPSR, UC_ARM_REG_INVALID};

const size_t ELF_NGREG = (sizeof(struct pt_regs) / sizeof(arion_lnx_type::elf_greg_t));
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct arion_lnx_type::elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};

struct fp_reg
{
    unsigned int sign1 : 1;
    unsigned int unused : 15;
    unsigned int sign2 : 1;
    unsigned int exponent : 14;
    unsigned int j : 1;
    unsigned int mantissa1 : 31;
    unsigned int mantissa0 : 32;
};

struct user_fp
{
    struct fp_reg fpregs[8];
    unsigned int fpsr : 32;
    unsigned int fpcr : 32;
    unsigned char ftype[8];
    unsigned int init_flag;
};

inline std::vector<arion::REG> uc_fp_regs = {UC_ARM_REG_D0, UC_ARM_REG_D1, UC_ARM_REG_D2, UC_ARM_REG_D3,
                                             UC_ARM_REG_D4, UC_ARM_REG_D5, UC_ARM_REG_D6, UC_ARM_REG_D7};

typedef struct user_fp elf_fpregset_t;

class ArchManagerLinuxARM : public arion_arm::ArchManagerARM, public arion::LinuxArchManager
{
  private:
    uint64_t pack_fp_reg(const struct arion_lnx_arm::fp_reg *r);
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_arm

#endif // ARION_LNX_ARCH_ARM_HPP
