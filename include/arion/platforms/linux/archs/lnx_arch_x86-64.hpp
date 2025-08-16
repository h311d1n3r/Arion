#ifndef ARION_LNX_ARCH_X8664_HPP
#define ARION_LNX_ARCH_X8664_HPP

#include <arion/archs/arch_x86-64.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_x86_64
{

struct user_regs_struct
{
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long bp;
    unsigned long bx;
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long ax;
    unsigned long cx;
    unsigned long dx;
    unsigned long si;
    unsigned long di;
    unsigned long orig_ax;
    unsigned long ip;
    unsigned long cs;
    unsigned long flags;
    unsigned long sp;
    unsigned long ss;
    unsigned long fs_base;
    unsigned long gs_base;
    unsigned long ds;
    unsigned long es;
    unsigned long fs;
    unsigned long gs;
};

inline std::vector<arion::REG> uc_user_regs = {
    UC_X86_REG_R15,    UC_X86_REG_R14, UC_X86_REG_R13, UC_X86_REG_R12,     UC_X86_REG_RBP,     UC_X86_REG_RBX,
    UC_X86_REG_R11,    UC_X86_REG_R10, UC_X86_REG_R9,  UC_X86_REG_R8,      UC_X86_REG_RAX,     UC_X86_REG_RCX,
    UC_X86_REG_RDX,    UC_X86_REG_RSI, UC_X86_REG_RDI, UC_X86_REG_INVALID, UC_X86_REG_RIP,     UC_X86_REG_CS,
    UC_X86_REG_RFLAGS, UC_X86_REG_RSP, UC_X86_REG_SS,  UC_X86_REG_FS_BASE, UC_X86_REG_GS_BASE, UC_X86_REG_DS,
    UC_X86_REG_ES,     UC_X86_REG_FS,  UC_X86_REG_GS};

const size_t ELF_NGREG = (sizeof(struct user_regs_struct) / sizeof(elf_greg_t));
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};

} // namespace arion_lnx_x86_64

class ArchManagerLinuxX8664 : public ArchManagerX8664, public LinuxArchManager
{
  public:
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
};

#endif // ARION_LNX_ARCH_X8664_HPP
