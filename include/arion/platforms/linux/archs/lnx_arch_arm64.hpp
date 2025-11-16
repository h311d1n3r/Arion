#ifndef ARION_LNX_ARCH_ARM64_HPP
#define ARION_LNX_ARCH_ARM64_HPP

#include <arion/archs/arch_arm64.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_arm64
{

/// Structure representing the user-space view of AArch64 general registers saved by the kernel.
struct user_pt_regs
{
    /// General-purpose registers X0 to X30.
    __u64 regs[31];
    /// Stack Pointer (SP).
    __u64 sp;
    /// Program Counter (PC).
    __u64 pc;
    /// Process State (PSTATE) register.
    __u64 pstate;
};

/// A vector mapping the `user_pt_regs` fields to ARION's universal register enumeration (`arion::REG`).
inline std::vector<arion::REG> uc_user_pt_regs = {
    UC_ARM64_REG_X0,  UC_ARM64_REG_X1,  UC_ARM64_REG_X2,  UC_ARM64_REG_X3,    UC_ARM64_REG_X4,  UC_ARM64_REG_X5,
    UC_ARM64_REG_X6,  UC_ARM64_REG_X7,  UC_ARM64_REG_X8,  UC_ARM64_REG_X9,    UC_ARM64_REG_X10, UC_ARM64_REG_X11,
    UC_ARM64_REG_X12, UC_ARM64_REG_X13, UC_ARM64_REG_X14, UC_ARM64_REG_X15,   UC_ARM64_REG_X16, UC_ARM64_REG_X17,
    UC_ARM64_REG_X18, UC_ARM64_REG_X19, UC_ARM64_REG_X20, UC_ARM64_REG_X21,   UC_ARM64_REG_X22, UC_ARM64_REG_X23,
    UC_ARM64_REG_X24, UC_ARM64_REG_X25, UC_ARM64_REG_X26, UC_ARM64_REG_X27,   UC_ARM64_REG_X28, UC_ARM64_REG_X29,
    UC_ARM64_REG_X30, UC_ARM64_REG_SP,  UC_ARM64_REG_PC,  UC_ARM64_REG_PSTATE};

/// Structure representing a frame record in the AArch64 stack unwind mechanism.
struct frame_record
{
    /// Frame Pointer (FP/X29).
    uint64_t fp;
    /// Link Register (LR/X30).
    uint64_t lr;
};

/// Structure combining a frame record with additional metadata.
struct frame_record_meta
{
    /// The frame pointer and link register values.
    struct frame_record record;
    /// Metadata type information.
    uint64_t type;
};

/// Structure representing the full kernel-internal AArch64 register state.
struct pt_regs
{
    /// Union to access user registers either as a block or individually.
    union {
        /// The standard user register set.
        struct user_pt_regs user_regs;
        /// Anonymous structure for individual register access.
        struct
        {
            uint64_t regs[31];
            uint64_t sp;
            uint64_t pc;
            uint64_t pstate;
        };
    };
    /// Original value of X0 before a syscall.
    uint64_t orig_x0;
    /// The system call number.
    int32_t syscallno;
    /// Performance Monitoring Register (PMR).
    uint32_t pmr;

    /// Security Device-Enabled Interrupts (SDEI) translation table base register 1.
    uint64_t sdei_ttbr1;
    /// Stack frame metadata.
    struct frame_record_meta stackframe;

    /// Lock dependency tracking for hard interrupts.
    uint64_t lockdep_hardirqs;
    /// RCU (Read-Copy-Update) exit counter.
    uint64_t exit_rcu;
};

/// The number of 64-bit words (general registers) in the user-visible `user_pt_regs` structure, used for ELF notes.
const size_t ELF_NGREG = (sizeof(struct user_pt_regs) / sizeof(arion_lnx_type::elf_greg_t));
/// Definition of the general-purpose register set type for ELF notes (NT_PRSTATUS).
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

/// Structure for a process status ELF note (NT_PRSTATUS) for AArch64, containing general registers.
struct elf_prstatus
{
    /// Common process status fields (pid, signal info, etc.).
    struct arion_lnx_type::elf_prstatus_common common;
    /// The general-purpose registers captured.
    elf_gregset_t pr_reg;
    /// Flag indicating whether floating-point registers are valid.
    int pr_fpvalid;
};

/// Structure representing the AArch64 Floating-Point and SIMD (FPSIMD) state.
struct user_fpsimd_state
{
    /// Vector registers V0 to V31 (128-bit each).
    __uint128_t vregs[32];
    /// Floating-Point Status Register.
    __u32 fpsr;
    /// Floating-Point Control Register.
    __u32 fpcr;
    /// Reserved padding.
    __u32 __reserved[2];
};

/// A vector mapping the FPSIMD vector registers to ARION's universal register enumeration (`arion::REG`).
inline std::vector<arion::REG> uc_fpsimd_regs = {
    UC_ARM64_REG_V0,  UC_ARM64_REG_V1,  UC_ARM64_REG_V2,  UC_ARM64_REG_V3,  UC_ARM64_REG_V4,  UC_ARM64_REG_V5,
    UC_ARM64_REG_V6,  UC_ARM64_REG_V7,  UC_ARM64_REG_V8,  UC_ARM64_REG_V9,  UC_ARM64_REG_V10, UC_ARM64_REG_V11,
    UC_ARM64_REG_V12, UC_ARM64_REG_V13, UC_ARM64_REG_V14, UC_ARM64_REG_V15, UC_ARM64_REG_V16, UC_ARM64_REG_V17,
    UC_ARM64_REG_V18, UC_ARM64_REG_V19, UC_ARM64_REG_V20, UC_ARM64_REG_V21, UC_ARM64_REG_V22, UC_ARM64_REG_V23,
    UC_ARM64_REG_V24, UC_ARM64_REG_V25, UC_ARM64_REG_V26, UC_ARM64_REG_V27, UC_ARM64_REG_V28, UC_ARM64_REG_V29,
    UC_ARM64_REG_V30, UC_ARM64_REG_V31};

/// Definition of the floating-point register set type for ELF notes (NT_FPREGSET).
typedef struct user_fpsimd_state elf_fpregset_t;

/// Linux AArch64 architecture manager class for Arion.
class ArchManagerLinuxARM64 : public arion_arm64::ArchManagerARM64, public arion::LinuxArchManager
{
  private:
    /**
     * Converts the raw ELF `elf_prstatus` data (general registers) into an Arion register map.
     * @param[in] prstatus Raw byte vector containing the `elf_prstatus` structure.
     * @return A map of Arion's universal register IDs to their 64-bit values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    /**
     * Converts the raw ELF `elf_fpregset_t` data (floating-point/SIMD registers) into an Arion register map.
     * @param[in] fpregset Raw byte vector containing the `elf_fpregset_t` structure.
     * @return A map of Arion's universal register IDs to their 64-bit or 128-bit values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_arm64

#endif // ARION_LNX_ARCH_ARM64_HPP
