#ifndef ARION_ARCH_MANAGER_HPP
#define ARION_ARCH_MANAGER_HPP

#include <arion/capstone/capstone.h>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/keystone/keystone.h>
#include <arion/unicorn/unicorn.h>
#include <arion/unicorn/x86.h>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

/// If this flag is set, the loader will map the VVAR segment
#define ARION_VVAR_PRESENT (1 << 0)
/// If this flag is set, the loader will map the VDSO segment
#define ARION_VDSO_PRESENT (1 << 1)
/// If this flag is set, the loader will map the VSYSCALL segment
#define ARION_VSYSCALL_PRESENT (1 << 2)
/// If this flag is set, the loader will map the ARM_TRAPS segment
#define ARION_ARM_TRAPS_PRESENT (1 << 3)

/// Size of an entry in the VSYSCALL segment
#define ARION_VSYSCALL_ENTRY_SZ 1024

namespace arion
{
/// Flags telling the loader which architecture specific segments should stand in memory.
using KERNEL_SEG_FLAGS = uint8_t;

class Arion;

/// Unicorn PC and SP registers for genericity.
struct ARION_EXPORT ABI_REGISTERS
{
    /// Unicorn PC register.
    REG pc;
    /// Unicorn SP register.
    REG sp;
    /**
     * Builder for ABI_REGISTERS instances.
     * @param[in] Unicorn PC register.
     * @param[in] Unicorn SP register.
     */
    ABI_REGISTERS(REG pc, REG sp) : pc(pc), sp(sp) {};
};

/// Unicorn registers involved in calling convention.
struct ARION_EXPORT ABI_CALLING_CONVENTION
{
    /// Unicorn register handling the call return value.
    REG ret_reg;
    /// Unicorn registers handling the call parameters.
    std::vector<REG> param_regs;
    /*
     * Builder for ABI_CALLING_CONVENTION instances.
     * @param[in] ret_reg Unicorn register handling the call return value.
     * @param[in] param_regs Unicorn registers handling the call parameters.
     */
    ABI_CALLING_CONVENTION(REG ret_reg, std::vector<REG> param_regs)
        : ret_reg(ret_reg), param_regs(std::move(param_regs)) {};
};

/// Unicorn registers involved in syscalling convention.
struct ARION_EXPORT ABI_SYSCALLING_CONVENTION
{
    /// Unicorn register handling the syscall number.
    REG sysno_reg;
    /// Unicorn register handling the syscall return value.
    REG ret_reg;
    /// Unicorn registers handling the syscall parameters.
    std::vector<REG> sys_param_regs;
    /**
     * Builder for ABI_SYSCALLING_CONVENTION instances.
     * @param[in] sysno_reg Unicorn register handling the syscall number.
     * @param[in] ret_reg Unicorn register handling the syscall return value.
     * @param[in] sys_param_regs Unicorn registers handling the syscall parameters.
     */
    ABI_SYSCALLING_CONVENTION(REG sysno_reg, REG ret_reg, std::vector<REG> sys_param_regs)
        : sysno_reg(sysno_reg), ret_reg(ret_reg), sys_param_regs(std::move(sys_param_regs)) {};
};

/// Multiple architecture specific attributes, grouped in a structure for genericity purpose.
struct ARION_EXPORT ARCH_ATTRIBUTES
{
    /// Arion CPU architecture.
    CPU_ARCH arch;
    /// Size in bits of the general-purpose registers.
    uint16_t arch_sz;
    /// Size in bytes of a pointer.
    size_t ptr_sz;
    /// HWCAP to be inserted in Auxiliary Vector (AUXV).
    uint32_t hwcap;
    /// HWCAP2 to be inserted in Auxiliary Vector (AUXV).
    uint32_t hwcap2;
    /// Flags telling the loader which kernel segments should be mapped in memory.
    KERNEL_SEG_FLAGS seg_flags;
    /// Unicorn IP and SP registers for genericity.
    ABI_REGISTERS regs;
    /// Unicorn registers involved in calling convention.
    ABI_CALLING_CONVENTION calling_conv;
    /// Unicorn registers involved in syscalling convention.
    ABI_SYSCALLING_CONVENTION syscalling_conv;
    /// A map identifying a syscall name given its number.
    std::map<uint64_t, std::string> name_by_syscall_no;
    /**
     * Builder for ARCH_ATTRIBUTES instances.
     * @param[in] arch Arion CPU architecture.
     * @param[in] arch_sz Size in bits of the general-purpose registers.
     * @param[in] ptr_sz Size in bytes of a pointer.
     * @param[in] hwcap HWCAP to be inserted in Auxiliary Vector (AUXV).
     * @param[in] hwcap2 HWCAP2 to be inserted in Auxiliary Vector (AUXV).
     * @param[in] seg_flags Flags telling the loader which kernel segments should be mapped in memory.
     * @param[in] regs Unicorn IP and SP registers for genericity.
     * @param[in] calling_conv Unicorn registers involved in calling convention.
     * @param[in] syscalling_conv Unicorn registers involved in syscalling convention.
     * @param[in] name_by_syscall_no A map identifying a syscall name given its number.
     */
    ARCH_ATTRIBUTES(CPU_ARCH arch, uint16_t arch_sz, size_t ptr_sz, uint32_t hwcap, uint32_t hwcap2,
                    KERNEL_SEG_FLAGS seg_flags, ABI_REGISTERS regs, ABI_CALLING_CONVENTION calling_conv,
                    ABI_SYSCALLING_CONVENTION syscalling_conv, std::map<uint64_t, std::string> &name_by_syscall_no)
        : arch(arch), arch_sz(arch_sz), ptr_sz(ptr_sz), seg_flags(seg_flags), hwcap(hwcap), hwcap2(hwcap2), regs(regs),
          calling_conv(calling_conv), syscalling_conv(syscalling_conv), name_by_syscall_no(name_by_syscall_no) {};
};

/// Fields for Interrupt Descriptor Tables.
enum ARION_EXPORT CPU_INTR
{
    // x86 interrupts
    DIVIDE_ERROR,                 ///< Divide-by-zero error.
    DEBUG_EXCEPTION,              ///< Debug exception.
    NON_MASKABLE_INTR,            ///< Non-maskable interrupt.
    BREAKPOINT,                   ///< Breakpoint exception.
    OVERFLOW,                     ///< Overflow exception.
    BOUND_RANGE_EXCEEDED,         ///< Bound range exceeded.
    INVALID_OPCODE,               ///< Invalid opcode exception.
    DEVICE_NOT_AVAILABLE,         ///< Device not available.
    DOUBLE_FAULT,                 ///< Double fault.
    COPROCESSOR_SEGMENT_OVERRUN,  ///< Coprocessor segment overrun.
    INVALID_TSS,                  ///< Invalid Task State Segment.
    SEGMENT_NOT_PRESENT,          ///< Segment not present.
    STACK_SEGMENT_FAULT,          ///< Stack segment fault.
    GENERAL_PROTECTION_FAULT,     ///< General protection fault.
    PAGE_FAULT,                   ///< Page fault.
    RESERVED,                     ///< Reserved interrupt vector.
    X87_FLOATING_POINT_EXCEPTION, ///< x87 Floating-point exception.
    ALIGNMENT_CHECK,              ///< Alignment check.
    MACHINE_CHECK,                ///< Machine check.
    SIMD_FLOATING_POINT_ERROR,    ///< SIMD Floating-point exception.

    // ARM interrupts
    UDEF, ///< Undefined instruction.
    // SWI
    PREFETCH_ABORT, ///< Prefetch abort.
    DATA_ABORT,     ///< Data abort.
    IRQ,            ///< Normal interrupt request.
    FIQ,            ///< Fast interrupt request.
    BKPT,           ///< Breakpoint.
    EXCEPTION_EXIT, ///< Exception exit handler.
    KERNEL_TRAP,    ///< Kernel trap.
    HVC,            ///< Hypervisor call.
    HYP_TRAP,       ///< Hypervisor trap.
    SMC,            ///< Secure monitor call.
    VIRQ,           ///< Virtual IRQ.
    VFIQ,           ///< Virtual FIQ.
    SEMIHOST,       ///< Semihosting trap.
    NOCP,           ///< No coprocessor present.
    INVSTATE,       ///< Invalid processor state.
    STKOF,          ///< Stack overflow.
    LAZYFP,         ///< Lazy floating-point state preservation.
    LSERR,          ///< Lock-step error.
    UNALIGNED       ///< Unaligned access.
};

/// A class responsible for performing architecture specific operations. This class is abstract and its subclasses must
/// implement a specific architecture.
class ARION_EXPORT ArchManager
{
  private:
    /// A map identifying a signal number given a cpu interruption.
    static std::map<CPU_INTR, int> signo_by_intr;

  protected:
    /// The Arion instanced associated to this instance.
    std::weak_ptr<Arion> arion;
    /// The Unicorn engine associated with this instance.
    uc_engine *uc;
    /// The Keystone engine associated with this instance.
    std::vector<ks_engine *> ks;
    /// The Capstone engine associated with this instance.
    std::vector<csh *> cs;
    /// Multiple architecture specific attributes, grouped in a structure for genericity purpose.
    std::shared_ptr<ARCH_ATTRIBUTES> attrs;
    /// Unicorn registers by their name.
    std::map<std::string, REG> arch_regs;
    /// Unicorn registers sizes.
    std::map<REG, uint8_t> arch_regs_sz;
    /// Unicorn registers making up the context to save and restore.
    std::vector<REG> ctxt_regs;
    /// Interrupt Descriptor Table for the CPU.
    std::map<uint64_t, CPU_INTR> cpu_idt;
    /// True if the ArchManager subclass uses hook_intr to intercept syscalls.
    bool hooks_intr;
    /**
     * Builder for ArchManager instances.
     * @param[in] attrs Multiple architecture specific attributes, grouped in a structure for genericity purpose.
     * @param[in] arch_regs Unicorn registers by their name.
     * @param[in] arch_regs_sz Unicorn registers sizes.
     * @param[in] ctxt_regs Unicorn registers making up the context to save and restore.
     * @param[in] Interrupt Descriptor Table for the CPU.
     * @param[in] hooks_intr True if the ArchManager subclass uses hook_intr to detect syscalls.
     */
    ArchManager(std::shared_ptr<ARCH_ATTRIBUTES> attrs, std::map<std::string, REG> arch_regs,
                std::map<REG, uint8_t> arch_regs_sz, std::vector<REG> ctxt_regs, std::map<uint64_t, CPU_INTR> cpu_idt,
                bool hooks_intr)
        : attrs(attrs), arch_regs(arch_regs), arch_regs_sz(arch_regs_sz), ctxt_regs(ctxt_regs), cpu_idt(cpu_idt),
          hooks_intr(hooks_intr) {};

  public:
    /*
     * Destructor for ArchManager instances.
     */
    virtual ~ArchManager() = default;
    /**
     * Instanciates and initializes new ArchManager objects with some parameters.
     * @param[in] arion The Arion instance associated with this instance.
     * @param[in] arch Arion CPU architecture.
     * @param[in] platform Arion platform on which the emulation occurs.
     * @return A new ArchManager instance.
     */
    static std::unique_ptr<ArchManager> initialize(std::weak_ptr<Arion> arion, CPU_ARCH arch,
                                                   PLATFORM platform = PLATFORM::UNKNOWN_PLATFORM);
    /**
     * Retrieves a signal number from a CPU interrupt based on the Interrupt Descriptor Table for the architecture.
     * @param[in] intr The CPU interrupt.
     * @return The signal number.
     */
    static int get_signal_from_intr(CPU_INTR intr);
    /**
     * Retrieves the list of architecture specific attributes.
     * @return The list of architecture specific attributes.
     */
    std::shared_ptr<ARCH_ATTRIBUTES> ARION_EXPORT get_attrs();
    /**
     * Checks whether the ArchManager subclass uses hook_intr to intercept syscalls.
     * @return True if the ArchManager subclass uses hook_intr to intercept syscalls.
     */
    bool does_hook_intr();
    /**
     * Retrieves a syscall name by its number.
     * @param[in] syscall_no The syscall number.
     * @return The syscall name.
     */
    std::string ARION_EXPORT get_name_by_syscall_no(uint64_t syscall_no);
    /**
     * Checks whether this architecture has a syscall with a given name.
     * @param[in] name The syscall name.
     * @return True if this architecture has a syscall with a given name.
     */
    bool ARION_EXPORT has_syscall_with_name(std::string name);
    /**
     * Retrieves a syscall number by its name.
     * @param[in] name The syscall name.
     * @return The syscall number.
     */
    uint64_t ARION_EXPORT get_syscall_no_by_name(std::string name);
    /**
     * Retrieves the list of registers making up the context to save and restore.
     * @return The list of registers.
     */
    std::vector<REG> ARION_EXPORT get_context_regs();
    /**
     * During emulation, dumps values of registers making up the context inside a map.
     * @return A map identifying a value by its associated register.
     */
    std::unique_ptr<std::map<REG, RVAL>> ARION_EXPORT dump_regs();
    /**
     * During emulation, loads values of registers making up the context from a map.
     * @param[in] regs A map identifying a value by its associated register.
     */
    void ARION_EXPORT load_regs(std::unique_ptr<std::map<REG, RVAL>> regs);
    /**
     * Checks whether this CPU architecture has a given Interrupt Descriptor Table entry.
     * @param[in] intno The interrupt number.
     * @return True if this CPU architecture has a given Interrupt Descriptor Table entry.
     */
    bool ARION_EXPORT has_idt_entry(uint64_t intno);
    /**
     * Retrieves a Arion Interrupt Descriptor Table value from an interrupt number.
     * @param[in] intno The interrupt number.
     * @return The Arion IDT value.
     */
    CPU_INTR ARION_EXPORT get_idt_entry(uint64_t intno);
    /**
     * Initializes a map of values associated to registers, with a PC and SP value. This method is used when
     * instanciating a new thread, where only these two registers are initialized.
     * @param[in] pc The Unicorn PC register.
     * @param[in] sp The Unicorn SP register.
     * @return The map of values associated to registers.
     */
    std::unique_ptr<std::map<REG, RVAL>> init_thread_regs(ADDR pc, ADDR sp);
    /**
     * Retrieves a Keystone engine associated with this instance, based on the current mode of the CPU.
     * @return The Keystone engine.
     */
    virtual ks_engine ARION_EXPORT *curr_ks() = 0;
    /**
     * Retrieves a Capstone engine associated with this instance, based on the current mode of the CPU.
     * @return The Capstone engine.
     */
    virtual csh ARION_EXPORT *curr_cs() = 0;
    /**
     * Prepares the ArchManager for emulation.
     */
    virtual void setup() = 0;
    /**
     * Retrieves the current Thread Local Storage (TLS) address from the emulation context.
     * @return The TLS address.
     */
    virtual ADDR ARION_EXPORT dump_tls() = 0;
    /**
     * Defines a Thread Local Storage (TLS) address to apply to the emulation.
     * @param[in] new_tls The new TLS address.
     */
    virtual void ARION_EXPORT load_tls(ADDR new_tls) = 0;
    /**
     * Performs architecture specific operations each time emulation starts.
     * @param[in, out] start The emulation start address.
     */
    virtual void prerun_hook(ADDR &start) {};
    /**
     * Reads a register value from the CPU context during emulation.
     * @tparam T A RVAL type large enough to store the register value.
     * @param[in] reg The Unicorn register.
     * @return The register value.
     */
    template <typename T> T ARION_EXPORT read_reg(REG reg)
    {
        auto reg_sz_it = this->arch_regs_sz.find(reg);
        if (reg_sz_it == this->arch_regs_sz.end())
            throw arion_exception::NoRegWithValueException(reg);
        if (sizeof(T) < reg_sz_it->second)
            throw arion_exception::HeavierRegException(reg_sz_it->second, sizeof(T));
        T val;
        uc_err uc_reg_err = uc_reg_read(this->uc, reg, &val);
        if (uc_reg_err != UC_ERR_OK)
            throw arion_exception::UnicornRegReadException(uc_reg_err);
        return val;
    }
    /**
     * Reads a register value from the CPU context during emulation.
     * @tparam T A RVAL type large enough to store the register value.
     * @param[in] reg The register name.
     * @return The register value.
     */
    template <typename T> T ARION_EXPORT read_reg(std::string reg_name)
    {
        reg_name = str_to_uppercase(reg_name);
        auto reg_it = this->arch_regs.find(reg_name);
        if (reg_it == this->arch_regs.end())
            throw arion_exception::NoRegWithNameException(reg_name);
        return this->read_reg<T>(reg_it->second);
    }
    /**
     * Reads a general-purpose register value from the CPU context during emulation. The result is casted on a 64-bit
     * integer for convenience.
     * @param[in] reg The Unicorn register.
     * @return The register value.
     */
    uint64_t read_arch_reg(REG reg);
    /**
     * Writes a register value to the CPU context during emulation.
     * @tparam T A RVAL type large enough to store the register value.
     * @param[in] reg The Unicorn register.
     * @param[in] val The new register value.
     */
    template <typename T> void ARION_EXPORT write_reg(REG reg, T val)
    {
        auto reg_sz_it = this->arch_regs_sz.find(reg);
        if (reg_sz_it == this->arch_regs_sz.end())
            throw arion_exception::NoRegWithValueException(reg);
        if (sizeof(T) < reg_sz_it->second)
            throw arion_exception::HeavierRegException(reg_sz_it->second, sizeof(T));
        uc_err uc_reg_err = uc_reg_write(this->uc, reg, &val);
        if (uc_reg_err != UC_ERR_OK)
            throw arion_exception::UnicornRegWriteException(uc_reg_err);
    }
    /**
     * Writes a register value to the CPU context during emulation.
     * @tparam T A RVAL type large enough to store the register value.
     * @param[in] reg The register name.
     * @param[in] val The new register value.
     */
    template <typename T> void ARION_EXPORT write_reg(std::string reg_name, T val)
    {
        reg_name = str_to_uppercase(reg_name);
        auto reg_it = this->arch_regs.find(reg_name);
        if (reg_it == this->arch_regs.end())
            throw arion_exception::NoRegWithNameException(reg_name);
        this->write_reg(reg_it->second, val);
    }
    /**
     * Writes a general-purpose register value to the CPU context during emulation. The new value is casted on a
     * 64-bit integer for convenience.
     * @param[in] reg The Unicorn register.
     * @param[in] val The new register value.
     */
    void write_arch_reg(REG reg, uint64_t val);
};

}; // namespace arion

#endif // ARION_ARCH_MANAGER_HPP
