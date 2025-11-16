#ifndef ARION_LNX_ARCH_ARM_HPP
#define ARION_LNX_ARCH_ARM_HPP

#include <arion/archs/arch_arm.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_arm
{

/// Structure represents the ARM processor's register state as saved on the stack by the Linux kernel.
struct pt_regs
{
    /// Array containing the core general-purpose and system registers (R0-R15, SP, LR, PC, CPSR, etc.)
    unsigned long uregs[18];
};

/// A vector mapping the Linux `pt_regs` structure indices to ARION's universal register enumeration (`arion::REG`).
inline std::vector<arion::REG> uc_pt_regs = {
    UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4,   UC_ARM_REG_R5,
    UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10,  UC_ARM_REG_FP,
    UC_ARM_REG_IP, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_CPSR, UC_ARM_REG_INVALID};

/// The number of 32-bit words (general registers) in the `pt_regs` structure, used for ELF notes.
const size_t ELF_NGREG = (sizeof(struct pt_regs) / sizeof(arion_lnx_type::elf_greg_t));
/// Definition of the general-purpose register set type for ELF notes.
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

/// Structure for a process status ELF note (NT_PRSTATUS) for ARM, containing general registers.
struct elf_prstatus
{
    /// Common process status fields (pid, signal info, etc.).
    struct arion_lnx_type::elf_prstatus_common common;
    /// The general-purpose registers captured.
    elf_gregset_t pr_reg;
    /// Flag indicating whether floating-point registers are valid.
    int pr_fpvalid;
};

/// Structure representing a single ARM floating-point register (double-precision format).
struct fp_reg
{
    /// Sign bit of the mantissa.
    unsigned int sign1 : 1;
    /// Unused bits.
    unsigned int unused : 15;
    /// Sign bit of the exponent.
    unsigned int sign2 : 1;
    /// Exponent part.
    unsigned int exponent : 14;
    /// Integer bit in normalized representation.
    unsigned int j : 1;
    /// High 31 bits of the mantissa.
    unsigned int mantissa1 : 31;
    /// Low 32 bits of the mantissa.
    unsigned int mantissa0 : 32;
};

/// Structure representing the ARM floating-point unit (FPU) context as saved in the user area.
struct user_fp
{
    /// Array of 8 double-precision floating-point registers (D0-D7).
    struct fp_reg fpregs[8];
    /// Floating-Point Status Register.
    unsigned int fpsr : 32;
    /// Floating-Point Control Register.
    unsigned int fpcr : 32;
    /// Array of floating-point type markers.
    unsigned char ftype[8];
    /// Initialization flag for the FPU context.
    unsigned int init_flag;
};

/// A vector mapping the FPU registers to ARION's universal register enumeration (`arion::REG`).
inline std::vector<arion::REG> uc_fp_regs = {UC_ARM_REG_D0, UC_ARM_REG_D1, UC_ARM_REG_D2, UC_ARM_REG_D3,
                                             UC_ARM_REG_D4, UC_ARM_REG_D5, UC_ARM_REG_D6, UC_ARM_REG_D7};

/// Definition of the floating-point register set type for ELF notes (NT_FPREGSET).
typedef struct user_fp elf_fpregset_t;

/// Linux ARM architecture manager class for Arion.
class ArchManagerLinuxARM : public arion_arm::ArchManagerARM, public arion::LinuxArchManager
{
  private:
    /**
     * Packs the custom `fp_reg` structure into a standard 64-bit value (`uint64_t`).
     * This is typically used to normalize the FPU state for use in the emulator.
     * @param[in] r Pointer to the ARM floating-point register structure.
     * @return The 64-bit packed value of the floating-point register.
     */
    uint64_t pack_fp_reg(const struct arion_lnx_arm::fp_reg *r);
    /**
     * Converts the raw ELF `elf_prstatus` data (general registers) into an Arion register map.
     * @param[in] prstatus Raw byte vector containing the `elf_prstatus` structure.
     * @return A map of Arion's universal register IDs to their 64-bit values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    /**
     * Converts the raw ELF `elf_fpregset_t` data (floating-point registers) into an Arion register map.
     * @param[in] fpregset Raw byte vector containing the `elf_fpregset_t` structure.
     * @return A map of Arion's universal register IDs to their 64-bit values (`arion::RVAL`).
     */
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_arm

#endif // ARION_LNX_ARCH_ARM_HPP
