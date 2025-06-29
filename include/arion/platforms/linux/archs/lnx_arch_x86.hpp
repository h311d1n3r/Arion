#ifndef ARION_LNX_ARCH_X86_HPP
#define ARION_LNX_ARCH_X86_HPP

#include <arion/archs/arch_x86.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_x86
{
struct user_regs_struct
{
    unsigned long bx;
    unsigned long cx;
    unsigned long dx;
    unsigned long si;
    unsigned long di;
    unsigned long bp;
    unsigned long ax;
    unsigned long ds;
    unsigned long es;
    unsigned long fs;
    unsigned long gs;
    unsigned long orig_ax;
    unsigned long ip;
    unsigned long cs;
    unsigned long flags;
    unsigned long sp;
    unsigned long ss;
};

const size_t ELF_NGREG = (sizeof(struct user_regs_struct) / sizeof(elf_greg_t));
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};
} // namespace arion_lnx_x86

class ArchManagerLinuxX86 : public ArchManagerX86, public LinuxArchManager
{
  public:
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
};

#endif // ARION_LNX_ARCH_X86_HPP
