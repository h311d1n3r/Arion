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

const size_t ELF_NGREG = (sizeof(struct pt_regs) / sizeof(elf_greg_t));
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};

} // namespace arion_lnx_arm

class ArchManagerLinuxARM : public ArchManagerARM, public LinuxArchManager
{
  public:
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
};

#endif // ARION_LNX_ARCH_ARM_HPP
