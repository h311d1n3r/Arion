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

inline std::vector<arion::REG> uc_user_regs = {
    UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_EAX, UC_X86_REG_DS,  UC_X86_REG_ES,     UC_X86_REG_FS,  UC_X86_REG_GS,  UC_X86_REG_INVALID,
    UC_X86_REG_EIP, UC_X86_REG_CS,  UC_X86_REG_EFLAGS, UC_X86_REG_ESP, UC_X86_REG_SS};

const size_t ELF_NGREG = (sizeof(struct user_regs_struct) / sizeof(arion::elf_greg_t));
typedef arion::elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct arion::elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};

struct user_i387_struct
{
    long cwd;
    long swd;
    long twd;
    long fip;
    long fcs;
    long foo;
    long fos;
    long st_space[20];
};

inline std::vector<arion::REG> uc_st_space_regs = {UC_X86_REG_FP0, UC_X86_REG_FP1, UC_X86_REG_FP2, UC_X86_REG_FP3,
                                                   UC_X86_REG_FP4, UC_X86_REG_FP5, UC_X86_REG_FP6, UC_X86_REG_FP7};

typedef struct user_i387_struct elf_fpregset_t;

class ArchManagerLinuxX86 : public arion_x86::ArchManagerX86, public arion::LinuxArchManager
{
  private:
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_x86

#endif // ARION_LNX_ARCH_X86_HPP
