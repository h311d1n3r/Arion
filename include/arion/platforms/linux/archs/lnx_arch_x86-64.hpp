#ifndef ARION_LNX_ARCH_X8664_HPP
#define ARION_LNX_ARCH_X8664_HPP

#include <arion/archs/arch_x86-64.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_x86_64
{

/// Structure representing the x86-64 general-purpose registers as saved by the Linux kernel on the stack.
struct user_regs_struct
{
    /// R15 register.
    unsigned long r15;
    /// R14 register.
    unsigned long r14;
    /// R13 register.
    unsigned long r13;
    /// R12 register.
    unsigned long r12;
    /// Base Pointer register (RBP/BP).
    unsigned long bp;
    /// Base register (RBX/BX).
    unsigned long bx;
    /// R11 register.
    unsigned long r11;
    /// R10 register.
    unsigned long r10;
    /// R9 register.
    unsigned long r9;
    /// R8 register.
    unsigned long r8;
    /// Accumulator register (RAX/AX).
    unsigned long ax;
    /// Counter register (RCX/CX).
    unsigned long cx;
    /// Data register (RDX/DX).
    unsigned long dx;
    /// Source Index register (RSI/SI).
    unsigned long si;
    /// Destination Index register (RDI/DI).
    unsigned long di;
    /// Original value of AX/RAX before a syscall (used to detect syscall return).
    unsigned long orig_ax;
    /// Instruction Pointer register (RIP/IP).
    unsigned long ip;
    /// Code Segment selector (CS).
    unsigned long cs;
    /// Flags register (RFLAGS).
    unsigned long flags;
    /// Stack Pointer register (RSP/SP).
    unsigned long sp;
    /// Stack Segment selector (SS).
    unsigned long ss;
    /// FS Segment Base address.
    unsigned long fs_base;
    /// GS Segment Base address.
    unsigned long gs_base;
    /// Data Segment selector (DS).
    unsigned long ds;
    /// Extra Segment selector (ES).
    unsigned long es;
    /// FS Segment selector.
    unsigned long fs;
    /// GS Segment selector.
    unsigned long gs;
};

/// A vector mapping the `user_regs_struct` fields to ARION's universal register enumeration (`arion::REG`).
inline std::vector<arion::REG> uc_user_regs = {
    UC_X86_REG_R15,    UC_X86_REG_R14, UC_X86_REG_R13, UC_X86_REG_R12,     UC_X86_REG_RBP,     UC_X86_REG_RBX,
    UC_X86_REG_R11,    UC_X86_REG_R10, UC_X86_REG_R9,  UC_X86_REG_R8,      UC_X86_REG_RAX,     UC_X86_REG_RCX,
    UC_X86_REG_RDX,    UC_X86_REG_RSI, UC_X86_REG_RDI, UC_X86_REG_INVALID, UC_X86_REG_RIP,     UC_X86_REG_CS,
    UC_X86_REG_RFLAGS, UC_X86_REG_RSP, UC_X86_REG_SS,  UC_X86_REG_FS_BASE, UC_X86_REG_GS_BASE, UC_X86_REG_DS,
    UC_X86_REG_ES,     UC_X86_REG_FS,  UC_X86_REG_GS};

/// The number of 64-bit words (general registers) in the `user_regs_struct`, used for ELF notes.
const size_t ELF_NGREG = (sizeof(struct user_regs_struct) / sizeof(arion_lnx_type::elf_greg_t));
/// Definition of the general-purpose register set type for ELF notes (NT_PRSTATUS).
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

/// Structure for a process status ELF note (NT_PRSTATUS) for x86-64, containing general registers.
struct elf_prstatus
{
    /// Common process status fields (pid, signal info, etc.).
    struct arion_lnx_type::elf_prstatus_common common;
    /// The general-purpose registers captured.
    elf_gregset_t pr_reg;
    /// Flag indicating whether floating-point registers are valid.
    int pr_fpvalid;
};

/// Flag indicating whether floating-point registers are valid.
struct user_i387_struct
{
    /// FPU Control Word.
    unsigned short cwd;
    /// FPU Status Word.
    unsigned short swd;
    /// FPU Tag Word.
    unsigned short twd;

    /// FPU Opcode.
    unsigned short fop;
    /// FPU Instruction Pointer.
    __u64 rip;
    /// FPU Data Pointer.
    __u64 rdp;
    /// MXCSR register (SSE control/status).
    __u32 mxcsr;
    /// MXCSR mask.
    __u32 mxcsr_mask;
    /// FPU stack registers ST0-ST7 (80-bit extended precision).
    __u32 st_space[32];
    /// SSE/AVX registers XMM0-XMM15 (128-bit/256-bit).
    __u32 xmm_space[64];
    /// Padding for alignment.
    __u32 padding[24];
};

/// A vector mapping the FPU stack register space (`st_space`) to ARION's universal register enumeration.
inline std::vector<arion::REG> uc_st_space_regs = {UC_X86_REG_FP0, UC_X86_REG_FP1, UC_X86_REG_FP2, UC_X86_REG_FP3,
                                                   UC_X86_REG_FP4, UC_X86_REG_FP5, UC_X86_REG_FP6, UC_X86_REG_FP7};

/// A vector mapping the SSE/AVX register space (`xmm_space`) to ARION's universal register enumeration.
inline std::vector<arion::REG> uc_xmm_space_regs = {
    UC_X86_REG_XMM0,  UC_X86_REG_XMM1,  UC_X86_REG_XMM2,  UC_X86_REG_XMM3,  UC_X86_REG_XMM4,  UC_X86_REG_XMM5,
    UC_X86_REG_XMM6,  UC_X86_REG_XMM7,  UC_X86_REG_XMM8,  UC_X86_REG_XMM9,  UC_X86_REG_XMM10, UC_X86_REG_XMM11,
    UC_X86_REG_XMM12, UC_X86_REG_XMM13, UC_X86_REG_XMM14, UC_X86_REG_XMM15, UC_X86_REG_XMM16, UC_X86_REG_XMM17,
    UC_X86_REG_XMM18, UC_X86_REG_XMM19, UC_X86_REG_XMM20, UC_X86_REG_XMM21, UC_X86_REG_XMM22, UC_X86_REG_XMM23,
    UC_X86_REG_XMM24, UC_X86_REG_XMM25, UC_X86_REG_XMM26, UC_X86_REG_XMM27, UC_X86_REG_XMM28, UC_X86_REG_XMM29,
    UC_X86_REG_XMM30, UC_X86_REG_XMM31};

/// Definition of the floating-point register set type for ELF notes (NT_FPREGSET).
typedef struct user_i387_struct elf_fpregset_t;

/// Linux x86-64 architecture manager class for Arion.
class ArchManagerLinuxX8664 : public arion_x86_64::ArchManagerX8664, public arion::LinuxArchManager
{
  private:
    /**
     * Converts the raw ELF `elf_prstatus` data (general registers) into an Arion register map.
     * @param[in] prstatus Raw byte vector containing the `elf_prstatus` structure.
     * @return A map of Arion's universal register IDs to their 64-bit values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    /**
     * Converts the raw ELF `elf_fpregset_t` data (FPU/SSE/AVX registers) into an Arion register map.
     * @param[in] fpregset Raw byte vector containing the `elf_fpregset_t` structure.
     * @return A map of Arion's universal register IDs to their values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_x86_64

#endif // ARION_LNX_ARCH_X8664_HPP
