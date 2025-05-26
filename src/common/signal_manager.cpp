#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/signal_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <sys/wait.h>

using namespace arion;

std::map<int, std::string> SignalManager::signals = {
    // Synchronous signals
    {SIGFPE, "SIGFPE (Floating-point exception)"},
    {SIGILL, "SIGILL (Illegal instruction)"},
    {SIGSEGV, "SIGSEGV (Segmentation fault)"},
    {SIGBUS, "SIGBUS (Bus error)"},
    {SIGTRAP, "SIGTRAP (Trace or breakpoint trap)"},
    {SIGABRT, "SIGABRT (Abort)"},
    {SIGSYS, "SIGSYS (Bad system call)"},

    // Asynchronous signals
    {SIGHUP, "SIGHUP (Hangup)"},
    {SIGINT, "SIGINT (Interrupt)"},
    {SIGQUIT, "SIGQUIT (Quit)"},
    {SIGKILL, "SIGKILL (Killed)"},
    {SIGPIPE, "SIGPIPE (Broken Pipe)"},
    {SIGALRM, "SIGALRM (Alarm Clock)"},
    {SIGTERM, "SIGTERM (Terminated)"},
    {SIGUSR1, "SIGUSR1 (User Signal 1)"},
    {SIGUSR2, "SIGUSR2 (User Signal 2)"},
    {SIGCHLD, "SIGCHLD (Child Status)"},
    {SIGPWR, "SIGPWR (Power Fail/Restart)"},
    {SIGWINCH, "SIGWINCH (Window Size Change)"},
    {SIGURG, "SIGURG (Urgent Socket Condition)"},
    {SIGPOLL, "SIGPOLL (Socket I/O Possible)"},
    {SIGSTOP, "SIGSTOP (Stopped (signal))"},
    {SIGTSTP, "SIGTSTP (Stopped (user))"},
    {SIGCONT, "SIGCONT (Continued)"},
    {SIGTTIN, "SIGTTIN (Stopped (tty input))"},
    {SIGTTOU, "SIGTTOU (Stopped (tty output))"},
    {SIGVTALRM, "SIGVTALRM (Virtual Timer Expired)"},
    {SIGPROF, "SIGPROF (Profiling Timer Expired)"},
    {SIGXCPU, "SIGXCPU (CPU time limit exceeded)"},
    {SIGXFSZ, "SIGXFSZ (File size limit exceeded)"},
};

std::unique_ptr<SignalManager> SignalManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::make_unique<SignalManager>(arion);
}

void SignalManager::intr_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (arion->abi->has_idt_entry(intno))
    {
        CPU_INTR intr = arion->abi->get_idt_entry(intno);
        int signo = AbiManager::get_signal_from_intr(intr);
        arion->send_signal(arion->get_pid(), signo);
    }
}

bool SignalManager::invalid_memory_hook(std::shared_ptr<Arion> arion, uc_mem_type access, uint64_t addr, int size,
                                        int64_t val, void *user_data)
{
    arion->send_signal(arion->get_pid(), SIGSEGV);
    arion->sync_threads(); // Since Unicorn 2, returning true is not enough to gracefully recover from memory access
                           // error
    return true;
}

bool SignalManager::invalid_insn_hook(std::shared_ptr<Arion> arion, void *user_data)
{
    arion->send_signal(arion->get_pid(), SIGILL);
    arion->sync_threads(); // Since Unicorn 2, returning true is not enough to gracefully recover from invalid
                           // instruction error
    return true;
}

SignalManager::SignalManager(std::weak_ptr<Arion> arion) : arion(arion)
{
    std::shared_ptr<Arion> arion_ = arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");

    arion_->hooks->hook_intr(SignalManager::intr_hook);
    arion_->hooks->hook_mem_read_unmapped(SignalManager::invalid_memory_hook);
    arion_->hooks->hook_mem_write_unmapped(SignalManager::invalid_memory_hook);
    arion_->hooks->hook_mem_fetch_unmapped(SignalManager::invalid_memory_hook);
    arion_->hooks->hook_mem_read_prot(SignalManager::invalid_memory_hook);
    arion_->hooks->hook_mem_write_prot(SignalManager::invalid_memory_hook);
    arion_->hooks->hook_mem_fetch_prot(SignalManager::invalid_memory_hook);
    arion_->hooks->hook_insn_invalid(SignalManager::invalid_insn_hook);
}

void SignalManager::handle_sigchld(pid_t source_pid)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto wait_it = this->sigwait_list.find(arion->threads->get_running_tid());
    if (wait_it == this->sigwait_list.end())
        return;
    pid_t target_pid = wait_it->second;
    pid_t running_tid = arion->threads->get_running_tid();
    REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    std::weak_ptr<Arion> source_instance_weak = arion->get_group()->get_arion_instance(source_pid);
    std::shared_ptr<Arion> source_instance = source_instance_weak.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    if (target_pid == source_pid ||
        (target_pid == 0 && arion->has_child(source_pid) && arion->get_pgid() == source_instance->get_pgid()) ||
        (target_pid == -1 && arion->has_child(source_pid)) ||
        (target_pid < -1 && arion->has_child(source_pid) && source_instance->get_pgid() == -target_pid))
    {
        std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(running_tid));
        arion_t->stopped = false;
        if (arion_t->wait_status_addr)
        {
            arion->mem->write_val(arion_t->wait_status_addr, 0, sizeof(int));
            arion_t->wait_status_addr = 0;
        }
        arion->abi->write_arch_reg(ret_reg, source_pid);
        arion->threads->threads_map[running_tid] = std::move(arion_t);
        arion->remove_child(source_pid);
        this->sigwait_list.erase(running_tid);
    }
}

bool SignalManager::handle_sighandler(pid_t source_pid, int signo)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto sighandler_it = this->sighandlers.find(signo);
    if (sighandler_it == this->sighandlers.end())
        return false;
    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    REG sp_reg = arion->abi->get_attrs()->regs.sp;
    std::shared_ptr<struct ksigaction> handler = sighandler_it->second;
    std::shared_ptr<ARION_CONTEXT> ctxt = arion->context->save();
    ADDR curr_pc = arion->abi->read_arch_reg(pc_reg);
    arion->abi->write_arch_reg(pc_reg, (ADDR)handler->handler);
    // In both cases (with and without SA_SIGINFO), first parameter is signal number
    std::vector<REG> param_regs = arion->abi->get_attrs()->calling_conv.param_regs;
    arion->abi->write_arch_reg(param_regs.at(0), (uint64_t)signo);
    if (handler->flags & SA_SIGINFO)
    {
        this->ucontext_regs = std::move(
            arion->abi->dump_regs()); // For now, clone context registers instead of using ucontext from siginfo_t
        std::unique_ptr<siginfo_t> info = std::make_unique<siginfo_t>();
        memset(info.get(), 0, sizeof(siginfo_t));
        // TODO : Fill siginfo_t struct with missing fields
        info->si_signo = signo;
        info->si_pid = source_pid;
        uint64_t sp = arion->abi->read_arch_reg(sp_reg);
        sp -= sizeof(siginfo_t);
        arion->abi->write_arch_reg(sp_reg, sp);
        arion->mem->write(sp, (BYTE *)info.get(), sizeof(siginfo_t));
        arion->abi->write_arch_reg(param_regs.at(1), sp);
        arion->abi->write_arch_reg(param_regs.at(2), 0); // TODO : Provide ucontext here
    }
    arion->mem->stack_push(curr_pc);

    // Manipulating PC register directly for sighandler call disturbs current tb cache, we clear it
    uc_err uc_rm_cache_err = uc_ctl_remove_cache(arion->uc, curr_pc, curr_pc + 1);
    if (uc_rm_cache_err != UC_ERR_OK)
        throw UnicornCtlException(uc_rm_cache_err);

    std::shared_ptr<HOOK_ID> hook_id = std::make_shared<HOOK_ID>();
    *hook_id = arion->hooks->hook_code(
        [hook_id, ctxt](std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data) {
            arion->context->restore(ctxt);
            arion->hooks->unhook(*hook_id);
        },
        curr_pc, curr_pc);
    return true;
}

void SignalManager::handle_signal(pid_t source_pid, int signo)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto signal_it = this->signals.find(signo);
    if (signal_it == this->signals.end())
        throw UnknownSignalException(arion->get_pid(), arion->threads->get_running_tid(), signo);
    std::string signal_desc = signal_it->second;
    arion->logger->debug("SIGNAL : " + signal_desc);

    if (this->handle_sighandler(source_pid, signo))
        return;

    switch (signo)
    {
    // Synchronous signals
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    case SIGBUS:
    case SIGTRAP:
    case SIGABRT:
    case SIGSYS:
        throw UnhandledSyncSignalException(arion->get_pid(), arion->threads->get_running_tid(), signal_desc);
        break;

    // Asynchronous signals
    case SIGHUP:
        arion->stop();
        break;
    case SIGINT:
        arion->stop();
        break;
    case SIGQUIT:
        arion->stop();
        break;
    case SIGKILL:
        arion->stop();
        break;
    case SIGPIPE:
        arion->stop();
        break;
    case SIGALRM:
        arion->stop();
        break;
    case SIGTERM:
        arion->stop();
        break;
    case SIGUSR1:
        arion->stop();
        break;
    case SIGUSR2:
        arion->stop();
        break;
    case SIGCHLD:
        this->handle_sigchld(source_pid);
        break;
    case SIGPWR:
        break;
    case SIGWINCH:
        break;
    case SIGURG:
        break;
    case SIGPOLL:
        break;
    case SIGSTOP:
        arion->set_stopped();
        arion->sync_threads();
        break;
    case SIGTSTP:
        arion->set_stopped();
        arion->sync_threads();
        break;
    case SIGCONT:
        arion->set_resumed();
        break;
    case SIGTTIN:
        arion->set_stopped();
        arion->sync_threads();
        break;
    case SIGTTOU:
        arion->set_stopped();
        arion->sync_threads();
        break;
    case SIGVTALRM:
        arion->stop();
        break;
    case SIGPROF:
        arion->stop();
        break;
    case SIGXCPU:
        arion->stop();
        break;
    case SIGXFSZ:
        arion->stop();
        break;
    }
}

void SignalManager::wait_for_sig(pid_t target_tid, pid_t source_pid, ADDR wait_status_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (this->sigwait_list.find(target_tid) != this->sigwait_list.end())
        throw ThreadAlreadySigWaitingException(arion->get_pid(), target_tid);
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(target_tid));
    arion_t->stopped = true;
    this->sigwait_list[target_tid] = source_pid;
    arion_t->wait_status_addr = wait_status_addr;
    arion->threads->threads_map[target_tid] = std::move(arion_t);
}

bool SignalManager::has_sighandler(int signo)
{
    return this->sighandlers.find(signo) != this->sighandlers.end();
}

std::shared_ptr<struct ksigaction> SignalManager::get_sighandler(int signo)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto sighandler_it = this->sighandlers.find(signo);
    if (sighandler_it == this->sighandlers.end())
        throw NoSighandlerForSignalException(arion->get_pid(), arion->threads->get_running_tid(), signo);
    return sighandler_it->second;
}

void SignalManager::set_sighandler(int signo, std::shared_ptr<struct ksigaction> sighandler)
{
    this->sighandlers[signo] = sighandler;
}

bool SignalManager::sigreturn()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (!this->ucontext_regs)
        return false;
    arion->abi->load_regs(std::move(this->ucontext_regs));
    return true;
}
