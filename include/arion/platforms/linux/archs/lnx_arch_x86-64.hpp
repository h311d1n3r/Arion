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

const size_t ELF_NGREG = (sizeof(struct user_regs_struct) / sizeof(arion_lnx_type::elf_greg_t));
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct arion_lnx_type::elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};

struct user_i387_struct
{
    unsigned short cwd;
    unsigned short swd;
    unsigned short twd;

    unsigned short fop;
    __u64 rip;
    __u64 rdp;
    __u32 mxcsr;
    __u32 mxcsr_mask;
    __u32 st_space[32];
    __u32 xmm_space[64];
    __u32 padding[24];
};

inline std::vector<arion::REG> uc_st_space_regs = {UC_X86_REG_FP0, UC_X86_REG_FP1, UC_X86_REG_FP2, UC_X86_REG_FP3,
                                                   UC_X86_REG_FP4, UC_X86_REG_FP5, UC_X86_REG_FP6, UC_X86_REG_FP7};

inline std::vector<arion::REG> uc_xmm_space_regs = {
    UC_X86_REG_XMM0,  UC_X86_REG_XMM1,  UC_X86_REG_XMM2,  UC_X86_REG_XMM3,  UC_X86_REG_XMM4,  UC_X86_REG_XMM5,
    UC_X86_REG_XMM6,  UC_X86_REG_XMM7,  UC_X86_REG_XMM8,  UC_X86_REG_XMM9,  UC_X86_REG_XMM10, UC_X86_REG_XMM11,
    UC_X86_REG_XMM12, UC_X86_REG_XMM13, UC_X86_REG_XMM14, UC_X86_REG_XMM15, UC_X86_REG_XMM16, UC_X86_REG_XMM17,
    UC_X86_REG_XMM18, UC_X86_REG_XMM19, UC_X86_REG_XMM20, UC_X86_REG_XMM21, UC_X86_REG_XMM22, UC_X86_REG_XMM23,
    UC_X86_REG_XMM24, UC_X86_REG_XMM25, UC_X86_REG_XMM26, UC_X86_REG_XMM27, UC_X86_REG_XMM28, UC_X86_REG_XMM29,
    UC_X86_REG_XMM30, UC_X86_REG_XMM31};

typedef struct user_i387_struct elf_fpregset_t;

class ArchManagerLinuxX8664 : public arion_x86_64::ArchManagerX8664, public arion::LinuxArchManager
{
  private:
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_x86_64

#endif // ARION_LNX_ARCH_X8664_HPP
