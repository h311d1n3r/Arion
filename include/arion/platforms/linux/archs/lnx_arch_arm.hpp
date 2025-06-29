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
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
};

#endif // ARION_LNX_ARCH_ARM_HPP
