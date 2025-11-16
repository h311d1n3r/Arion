#ifndef ARION_LNX_ARCH_X86_HPP
#define ARION_LNX_ARCH_X86_HPP

#include <arion/archs/arch_x86.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_x86
{

/// Structure representing the x86 (32-bit) general-purpose registers as saved by the Linux kernel on the stack.
struct user_regs_struct
{
    /// Base register (EBX/BX).
    unsigned long bx;
    /// Counter register (ECX/CX).
    unsigned long cx;
    /// Data register (EDX/DX).
    unsigned long dx;
    /// Source Index register (ESI/SI).
    unsigned long si;
    /// Destination Index register (EDI/DI).
    unsigned long di;
    /// Base Pointer register (EBP/BP).
    unsigned long bp;
    /// Accumulator register (EAX/AX).
    unsigned long ax;
    /// Data Segment selector (DS).
    unsigned long ds;
    /// Extra Segment selector (ES).
    unsigned long es;
    /// FS Segment selector.
    unsigned long fs;
    /// GS Segment selector.
    unsigned long gs;
    /// Original value of AX/EAX before a syscall (used to detect syscall return).
    unsigned long orig_ax;
    /// Instruction Pointer register (EIP/IP).
    unsigned long ip;
    /// Code Segment selector (CS).
    unsigned long cs;
    /// Flags register (EFLAGS).
    unsigned long flags;
    /// Stack Pointer register (ESP/SP).
    unsigned long sp;
    /// Stack Segment selector (SS).
    unsigned long ss;
};

/// A vector mapping the `user_regs_struct` fields to ARION's universal register enumeration (`arion::REG`).
inline std::vector<arion::REG> uc_user_regs = {
    UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
    UC_X86_REG_EAX, UC_X86_REG_DS,  UC_X86_REG_ES,     UC_X86_REG_FS,  UC_X86_REG_GS,  UC_X86_REG_INVALID,
    UC_X86_REG_EIP, UC_X86_REG_CS,  UC_X86_REG_EFLAGS, UC_X86_REG_ESP, UC_X86_REG_SS};

/// The number of 32-bit words (general registers) in the `user_regs_struct`, used for ELF notes.
const size_t ELF_NGREG = (sizeof(struct user_regs_struct) / sizeof(arion_lnx_type::elf_greg_t));
/// Definition of the general-purpose register set type for ELF notes (NT_PRSTATUS).
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

/// Structure for a process status ELF note (NT_PRSTATUS) for x86 (32-bit), containing general registers.
struct elf_prstatus
{
    /// Common process status fields (pid, signal info, etc.).
    struct arion_lnx_type::elf_prstatus_common common;
    /// The general-purpose registers captured.
    elf_gregset_t pr_reg;
    /// Flag indicating whether floating-point registers are valid.
    int pr_fpvalid;
};

/// Structure representing the x86 Floating-Point Unit (FPU) state (user_i387_struct is the kernel's definition of FPU
/// state).
struct user_i387_struct
{
    /// FPU Control Word.
    long cwd;
    /// FPU Status Word.
    long swd;
    /// FPU Tag Word.
    long twd;
    /// FPU Instruction Pointer Offset.
    long fip;
    /// FPU Instruction Pointer Segment Selector.
    long fcs;
    /// FPU Data Pointer Offset.
    long foo;
    /// FPU Data Pointer Segment Selector.
    long fos;
    /// FPU stack registers ST0-ST7 (80-bit extended precision).
    long st_space[20];
};

/// A vector mapping the FPU stack register space (`st_space`) to ARION's universal register enumeration.
inline std::vector<arion::REG> uc_st_space_regs = {UC_X86_REG_FP0, UC_X86_REG_FP1, UC_X86_REG_FP2, UC_X86_REG_FP3,
                                                   UC_X86_REG_FP4, UC_X86_REG_FP5, UC_X86_REG_FP6, UC_X86_REG_FP7};

/// Definition of the floating-point register set type for ELF notes (NT_FPREGSET).
typedef struct user_i387_struct elf_fpregset_t;

/// Linux x86 architecture manager class for Arion.
class ArchManagerLinuxX86 : public arion_x86::ArchManagerX86, public arion::LinuxArchManager
{
  private:
    /**
     * Converts the raw ELF `elf_prstatus` data (general registers) into an Arion register map.
     * @param[in] prstatus Raw byte vector containing the `elf_prstatus` structure.
     * @return A map of Arion's universal register IDs to their 32-bit values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    /**
     * Converts the raw ELF `elf_fpregset_t` data (FPU registers) into an Arion register map.
     * @param[in] fpregset Raw byte vector containing the `elf_fpregset_t` structure.
     * @return A map of Arion's universal register IDs to their 80-bit FPU values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_x86

#endif // ARION_LNX_ARCH_X86_HPP
