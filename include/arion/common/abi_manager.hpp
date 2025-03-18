#ifndef ARION_ABI_MANAGER_HPP
#define ARION_ABI_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/capstone/capstone.h>
#include <cstdint>
#include <arion/keystone/keystone.h>
#include <map>
#include <memory>
#include <string>
#include <arion/unicorn/unicorn.h>
#include <arion/unicorn/x86.h>
#include <vector>

namespace arion
{
using KERNEL_SEG_FLAGS = uint8_t;
};

#define ARION_VVAR_PRESENT (1 << 0)
#define ARION_VDSO_PRESENT (1 << 1)
#define ARION_VSYSCALL_PRESENT (1 << 2)
#define ARION_ARM_TRAPS_PRESENT (1 << 3)

#define VSYSCALL_ENTRY_SZ 1024

class Arion;

struct ARION_EXPORT ABI_REGISTERS
{
    arion::REG pc;
    arion::REG sp;
    arion::REG tls;
    ABI_REGISTERS(arion::REG pc, arion::REG sp, arion::REG tls) : pc(pc), sp(sp), tls(tls) {};
};

struct ARION_EXPORT ABI_CALLING_CONVENTION
{
    arion::REG ret_reg;
    std::vector<arion::REG> param_regs;
    ABI_CALLING_CONVENTION(arion::REG ret_reg, std::vector<arion::REG> param_regs)
        : ret_reg(ret_reg), param_regs(std::move(param_regs)) {};
};

struct ARION_EXPORT ABI_SYSCALLING_CONVENTION
{
    arion::REG sysno_reg;
    arion::REG ret_reg;
    std::vector<arion::REG> sys_param_regs;
    ABI_SYSCALLING_CONVENTION(arion::REG sysno_reg, arion::REG ret_reg, std::vector<arion::REG> sys_param_regs)
        : sysno_reg(sysno_reg), ret_reg(ret_reg), sys_param_regs(std::move(sys_param_regs)) {};
};

struct ARION_EXPORT ABI_ATTRIBUTES
{
    arion::CPU_ARCH arch;
    std::string arch_name;
    uint16_t arch_sz;
    size_t ptr_sz;
    arion::KERNEL_SEG_FLAGS seg_flags;
    uint32_t hwcap;
    uint32_t hwcap2;
    ABI_REGISTERS regs;
    ABI_CALLING_CONVENTION calling_conv;
    ABI_SYSCALLING_CONVENTION syscalling_conv;
    std::map<uint64_t, std::string> name_by_syscall_no;
    ABI_ATTRIBUTES(arion::CPU_ARCH arch, std::string arch_name, uint16_t arch_sz, size_t ptr_sz, uint32_t hwcap,
                   uint32_t hwcap2, arion::KERNEL_SEG_FLAGS seg_flags, ABI_REGISTERS regs,
                   ABI_CALLING_CONVENTION calling_conv, ABI_SYSCALLING_CONVENTION syscalling_conv,
                   std::map<uint64_t, std::string> &name_by_syscall_no)
        : arch(arch), arch_name(arch_name), arch_sz(arch_sz), ptr_sz(ptr_sz), seg_flags(seg_flags), hwcap(hwcap),
          hwcap2(hwcap2), regs(regs), calling_conv(calling_conv), syscalling_conv(syscalling_conv),
          name_by_syscall_no(name_by_syscall_no) {};
};

enum ARION_EXPORT CPU_INTR
{
    // x86 interrupts
    DIVIDE_ERROR,
    DEBUG_EXCEPTION,
    NON_MASKABLE_INTR,
    BREAKPOINT,
    OVERFLOW,
    BOUND_RANGE_EXCEEDED,
    INVALID_OPCODE,
    DEVICE_NOT_AVAILABLE,
    DOUBLE_FAULT,
    COPROCESSOR_SEGMENT_OVERRUN,
    INVALID_TSS,
    SEGMENT_NOT_PRESENT,
    STACK_SEGMENT_FAULT,
    GENERAL_PROTECTION_FAULT,
    PAGE_FAULT,
    RESERVED,
    X87_FLOATING_POINT_EXCEPTION,
    ALIGNMENT_CHECK,
    MACHINE_CHECK,
    SIMD_FLOATING_POINT_ERROR,

    // ARM interrupts
    UDEF,
    // SWI,
    PREFETCH_ABORT,
    DATA_ABORT,
    IRQ,
    FIQ,
    BKPT,
    EXCEPTION_EXIT,
    KERNEL_TRAP,
    HVC,
    HYP_TRAP,
    SMC,
    VIRQ,
    VFIQ,
    SEMIHOST,
    NOCP,
    INVSTATE,
    STKOF,
    LAZYFP,
    LSERR,
    UNALIGNED
};

class ARION_EXPORT AbiManager
{
  private:
    static std::map<CPU_INTR, int> signo_by_intr;

  protected:
    std::weak_ptr<Arion> arion;
    uc_engine *uc;
    std::vector<ks_engine *> ks;
    std::vector<csh *> cs;
    std::shared_ptr<ABI_ATTRIBUTES> attrs;
    std::map<std::string, arion::REG> arch_regs;
    std::map<arion::REG, uint8_t> arch_regs_sz;
    std::vector<arion::REG> ctxt_regs;
    std::map<uint64_t, CPU_INTR> cpu_idt;
    bool hooks_intr;
    AbiManager(std::shared_ptr<ABI_ATTRIBUTES> attrs, std::map<std::string, arion::REG> arch_regs,
               std::map<arion::REG, uint8_t> arch_regs_sz, std::vector<arion::REG> ctxt_regs,
               std::map<uint64_t, CPU_INTR> cpu_idt, bool hooks_intr)
        : attrs(attrs), arch_regs(arch_regs), arch_regs_sz(arch_regs_sz), ctxt_regs(ctxt_regs), cpu_idt(cpu_idt),
          hooks_intr(hooks_intr) {};

  public:
    virtual ~AbiManager() = default;
    static std::unique_ptr<AbiManager> initialize(std::weak_ptr<Arion> arion, arion::CPU_ARCH arch);
    static int get_signal_from_intr(CPU_INTR intr);
    std::shared_ptr<ABI_ATTRIBUTES> ARION_EXPORT get_attrs();
    bool does_hook_intr();
    std::string ARION_EXPORT get_name_by_syscall_no(uint64_t syscall_no);
    bool ARION_EXPORT has_syscall_with_name(std::string name);
    uint64_t ARION_EXPORT get_syscall_no_by_name(std::string name);
    std::vector<arion::REG> ARION_EXPORT get_context_regs();
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> ARION_EXPORT dump_regs();
    void ARION_EXPORT load_regs(std::unique_ptr<std::map<arion::REG, arion::RVAL>> regs);
    bool ARION_EXPORT has_idt_entry(uint64_t intno);
    CPU_INTR ARION_EXPORT get_idt_entry(uint64_t intno);
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> init_thread_regs(arion::ADDR pc, arion::ADDR sp,
                                                                        arion::ADDR tls);
    virtual std::array<arion::BYTE, VSYSCALL_ENTRY_SZ> gen_vsyscall_entry(uint64_t syscall_no) = 0;
    virtual ks_engine ARION_EXPORT *curr_ks() = 0;
    virtual csh ARION_EXPORT *curr_cs() = 0;
    virtual void ARION_EXPORT setup() = 0;

    template <typename T> T ARION_EXPORT read_reg(arion::REG reg)
    {
        auto reg_sz_it = this->arch_regs_sz.find(reg);
        if (reg_sz_it == this->arch_regs_sz.end())
            throw NoRegWithValueException(reg);
        if (sizeof(T) < reg_sz_it->second)
            throw HeavierRegException(reg_sz_it->second, sizeof(T));
        T val;
        uc_err uc_reg_err = uc_reg_read(this->uc, reg, &val);
        if (uc_reg_err != UC_ERR_OK)
            throw UnicornRegReadException(uc_reg_err);
        return val;
    }

    template <typename T> T ARION_EXPORT read_reg(std::string reg_name)
    {
        reg_name = str_to_uppercase(reg_name);
        auto reg_it = this->arch_regs.find(reg_name);
        if (reg_it == this->arch_regs.end())
            throw NoRegWithNameException(reg_name);
        return this->read_reg<T>(reg_it->second);
    }

    uint64_t read_arch_reg(arion::REG reg);

    template <typename T> void ARION_EXPORT write_reg(arion::REG reg, T val)
    {
        auto reg_sz_it = this->arch_regs_sz.find(reg);
        if (reg_sz_it == this->arch_regs_sz.end())
            throw NoRegWithValueException(reg);
        if (sizeof(T) < reg_sz_it->second)
            throw HeavierRegException(reg_sz_it->second, sizeof(T));
        uc_err uc_reg_err = uc_reg_write(this->uc, reg, &val);
        if (uc_reg_err != UC_ERR_OK)
            throw UnicornRegWriteException(uc_reg_err);
    }

    template <typename T> void ARION_EXPORT write_reg(std::string reg_name, T val)
    {
        reg_name = str_to_uppercase(reg_name);
        auto reg_it = this->arch_regs.find(reg_name);
        if (reg_it == this->arch_regs.end())
            throw NoRegWithNameException(reg_name);
        this->write_reg(reg_it->second, val);
    }

    void write_arch_reg(arion::REG reg, uint64_t val);
};

#endif // ARION_ABI_MANAGER_HPP
