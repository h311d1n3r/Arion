#include <arion/archs/arch_arm.hpp>
#include <arion/archs/arch_arm64.hpp>
#include <arion/archs/arch_x86-64.hpp>
#include <arion/archs/arch_x86.hpp>
#include <arion/arion.hpp>
#include <arion/common/arch_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/platforms/linux/archs/lnx_arch_arm.hpp>
#include <arion/platforms/linux/archs/lnx_arch_arm64.hpp>
#include <arion/platforms/linux/archs/lnx_arch_x86-64.hpp>
#include <arion/platforms/linux/archs/lnx_arch_x86.hpp>
#include <arion/unicorn/unicorn.h>
#include <arion/unicorn/x86.h>
#include <cstdint>
#include <memory>
#include <sys/wait.h>

using namespace arion;

std::map<CPU_INTR, int> ArchManager::signo_by_intr = {
    // x86 interrupts
    {CPU_INTR::DIVIDE_ERROR, SIGFPE},
    {CPU_INTR::DEBUG_EXCEPTION, SIGTRAP},
    {CPU_INTR::NON_MASKABLE_INTR, SIGSEGV},
    {CPU_INTR::BREAKPOINT, SIGTRAP},
    {CPU_INTR::OVERFLOW, SIGFPE},
    {CPU_INTR::BOUND_RANGE_EXCEEDED, SIGFPE},
    {CPU_INTR::INVALID_OPCODE, SIGILL},
    {CPU_INTR::DEVICE_NOT_AVAILABLE, SIGFPE},
    {CPU_INTR::DOUBLE_FAULT, SIGSEGV},
    {CPU_INTR::COPROCESSOR_SEGMENT_OVERRUN, SIGFPE},
    {CPU_INTR::INVALID_TSS, SIGSEGV},
    {CPU_INTR::SEGMENT_NOT_PRESENT, SIGSEGV},
    {CPU_INTR::STACK_SEGMENT_FAULT, SIGSEGV},
    {CPU_INTR::GENERAL_PROTECTION_FAULT, SIGSEGV},
    {CPU_INTR::PAGE_FAULT, SIGSEGV},
    {CPU_INTR::RESERVED, SIGSEGV},
    {CPU_INTR::X87_FLOATING_POINT_EXCEPTION, SIGFPE},
    {CPU_INTR::ALIGNMENT_CHECK, SIGBUS},
    {CPU_INTR::MACHINE_CHECK, SIGABRT},
    {CPU_INTR::SIMD_FLOATING_POINT_ERROR, SIGFPE},

    // ARM interrupts
    {CPU_INTR::UDEF, SIGILL},
    {CPU_INTR::PREFETCH_ABORT, SIGBUS},
    {CPU_INTR::DATA_ABORT, SIGSEGV},
    {CPU_INTR::IRQ, SIGINT},
    {CPU_INTR::FIQ, SIGINT},
    {CPU_INTR::BKPT, SIGTRAP},
    {CPU_INTR::EXCEPTION_EXIT, SIGTERM},
    {CPU_INTR::KERNEL_TRAP, SIGSEGV},
    {CPU_INTR::HVC, SIGILL},
    {CPU_INTR::HYP_TRAP, SIGILL},
    {CPU_INTR::SMC, SIGSEGV},
    {CPU_INTR::VIRQ, SIGSEGV},
    {CPU_INTR::VFIQ, SIGSEGV},
    {CPU_INTR::SEMIHOST, SIGUSR1},
    {CPU_INTR::NOCP, SIGBUS},
    {CPU_INTR::INVSTATE, SIGSEGV},
    {CPU_INTR::STKOF, SIGSEGV},
    {CPU_INTR::LAZYFP, SIGSEGV},
    {CPU_INTR::LSERR, SIGBUS},
    {CPU_INTR::UNALIGNED, SIGBUS}};

std::unique_ptr<ArchManager> ArchManager::initialize(std::weak_ptr<Arion> arion, CPU_ARCH arch, PLATFORM platform)
{
    std::unique_ptr<ArchManager> manager;
    switch (platform)
    {
    case PLATFORM::LINUX:
        switch (arch)
        {
        case CPU_ARCH::X86_ARCH:
            manager = std::make_unique<ArchManagerLinuxX86>();
            break;
        case CPU_ARCH::X8664_ARCH:
            manager = std::make_unique<ArchManagerLinuxX8664>();
            break;
        case CPU_ARCH::ARM_ARCH:
            manager = std::make_unique<ArchManagerLinuxARM>();
            break;
        case CPU_ARCH::ARM64_ARCH:
            manager = std::make_unique<ArchManagerLinuxARM64>();
            break;
        default:
            throw UnsupportedCpuArchException();
        }
        break;
    case PLATFORM::UNKNOWN_PLATFORM:
    default:
        switch (arch)
        {
        case CPU_ARCH::X86_ARCH:
            manager = std::make_unique<ArchManagerX86>();
            break;
        case CPU_ARCH::X8664_ARCH:
            manager = std::make_unique<ArchManagerX8664>();
            break;
        case CPU_ARCH::ARM_ARCH:
            manager = std::make_unique<ArchManagerARM>();
            break;
        case CPU_ARCH::ARM64_ARCH:
            manager = std::make_unique<ArchManagerARM64>();
            break;
        default:
            throw UnsupportedCpuArchException();
        }
        break;
    }

    std::shared_ptr<Arion> arion_ = arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");

    manager->arion = arion;
    manager->uc = arion_->uc;
    manager->ks = arion_->ks;
    manager->cs = arion_->cs;
    manager->setup();
    return std::move(manager);
}

int ArchManager::get_signal_from_intr(CPU_INTR intr)
{
    auto signal_it = ArchManager::signo_by_intr.find(intr);
    if (signal_it == ArchManager::signo_by_intr.end())
        throw NoSignalForIntrException();
    return signal_it->second;
}

std::shared_ptr<ARCH_ATTRIBUTES> ArchManager::get_attrs()
{
    return this->attrs;
}

bool ArchManager::does_hook_intr()
{
    return this->hooks_intr;
}

std::string ArchManager::get_name_by_syscall_no(uint64_t syscall_no)
{
    std::map<uint64_t, std::string> name_by_syscall_no = this->attrs->name_by_syscall_no;
    if (name_by_syscall_no.find(syscall_no) == name_by_syscall_no.end())
        throw InvalidSyscallNoException(syscall_no);
    return name_by_syscall_no.at(syscall_no);
}

bool ArchManager::has_syscall_with_name(std::string name)
{
    std::map<ADDR, std::string> name_by_syscall_no = this->attrs->name_by_syscall_no;
    for (const auto &entry : name_by_syscall_no)
    {
        if (entry.second == name)
        {
            return true;
        }
    }
    return false;
}

uint64_t ArchManager::get_syscall_no_by_name(std::string name)
{
    std::map<ADDR, std::string> name_by_syscall_no = this->attrs->name_by_syscall_no;
    for (const auto &entry : name_by_syscall_no)
    {
        if (entry.second == name)
        {
            return entry.first;
        }
    }
    throw InvalidSyscallNameException(name);
}

std::vector<REG> ArchManager::get_context_regs()
{
    return this->ctxt_regs;
}

std::unique_ptr<std::map<REG, RVAL>> ArchManager::dump_regs()
{
    std::unique_ptr<std::map<REG, RVAL>> regs = std::make_unique<std::map<REG, RVAL>>();
    for (REG reg : this->ctxt_regs)
    {
        uint8_t reg_sz = this->arch_regs_sz.at(reg);
        if (reg_sz == 1)
            regs->operator[](reg).r8 = this->read_reg<RVAL8>(reg);
        else if (reg_sz == 2)
            regs->operator[](reg).r16 = this->read_reg<RVAL16>(reg);
        else if (reg_sz <= 4)
            regs->operator[](reg).r32 = this->read_reg<RVAL32>(reg);
        else if (reg_sz <= 8)
            regs->operator[](reg).r64 = this->read_reg<RVAL64>(reg);
        else if (reg_sz <= 16)
            regs->operator[](reg).r128 = this->read_reg<RVAL128>(reg);
        else if (reg_sz <= 32)
            regs->operator[](reg).r256 = this->read_reg<RVAL256>(reg);
        else
            regs->operator[](reg).r512 = this->read_reg<RVAL512>(reg);
    }
    return std::move(regs);
}

void ArchManager::load_regs(std::unique_ptr<std::map<REG, RVAL>> regs)
{
    for (auto reg_it : *regs)
    {
        uint8_t reg_sz = this->arch_regs_sz.at(reg_it.first);
        if (reg_sz == 1)
            this->write_reg<RVAL8>(reg_it.first, reg_it.second.r8);
        else if (reg_sz == 2)
            this->write_reg<RVAL16>(reg_it.first, reg_it.second.r16);
        else if (reg_sz <= 4)
            this->write_reg<RVAL32>(reg_it.first, reg_it.second.r32);
        else if (reg_sz <= 8)
            this->write_reg<RVAL64>(reg_it.first, reg_it.second.r64);
        else if (reg_sz <= 16)
            this->write_reg<RVAL128>(reg_it.first, reg_it.second.r128);
        else if (reg_sz <= 32)
            this->write_reg<RVAL256>(reg_it.first, reg_it.second.r256);
        else
            this->write_reg<RVAL512>(reg_it.first, reg_it.second.r512);
    }
}

bool ArchManager::has_idt_entry(uint64_t intno)
{
    return this->cpu_idt.find(intno) != this->cpu_idt.end();
}

CPU_INTR ArchManager::get_idt_entry(uint64_t intno)
{
    auto cpu_idt_it = this->cpu_idt.find(intno);
    if (cpu_idt_it == this->cpu_idt.end())
        throw NoIdtEntryWithIntnoException(intno);
    return cpu_idt_it->second;
}

std::unique_ptr<std::map<REG, RVAL>> ArchManager::init_thread_regs(ADDR pc, ADDR sp)
{
    std::unique_ptr<std::map<REG, RVAL>> regs = this->dump_regs();
    switch (this->attrs->arch_sz)
    {
    case 64:
        regs->operator[](this->attrs->regs.pc).r64 = pc;
        regs->operator[](this->attrs->regs.sp).r64 = sp;
        break;
    case 32:
        regs->operator[](this->attrs->regs.pc).r32 = pc;
        regs->operator[](this->attrs->regs.sp).r32 = sp;
        break;
    default:
        throw UnsupportedCpuArchException();
    }
    return std::move(regs);
}

uint64_t ArchManager::read_arch_reg(arion::REG reg)
{
    switch (this->attrs->arch_sz)
    {
    case 64:
        return this->read_reg<RVAL64>(reg);
    case 32:
        return this->read_reg<RVAL32>(reg);
    default:
        throw UnsupportedCpuArchException();
    }
}

void ArchManager::write_arch_reg(arion::REG reg, uint64_t val)
{
    switch (this->attrs->arch_sz)
    {
    case 64:
        this->write_reg<RVAL64>(reg, val);
        break;
    case 32:
        this->write_reg<RVAL32>(reg, val);
        break;
    default:
        throw UnsupportedCpuArchException();
    }
}
