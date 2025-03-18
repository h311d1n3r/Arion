#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/threading_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <asm/ldt.h>
#include <memory>
#include <sys/wait.h>
#include <arion/unicorn/unicorn.h>

using namespace arion;

std::vector<BYTE> serialize_arion_futex(ARION_FUTEX *arion_f)
{
    std::vector<BYTE> srz_futex;

    srz_futex.insert(srz_futex.end(), (BYTE *)&arion_f->futex_addr, (BYTE *)&arion_f->futex_addr + sizeof(ADDR));
    srz_futex.insert(srz_futex.end(), (BYTE *)&arion_f->futex_bitmask,
                     (BYTE *)&arion_f->futex_bitmask + sizeof(uint32_t));
    srz_futex.insert(srz_futex.end(), (BYTE *)&arion_f->tid, (BYTE *)&arion_f->tid + sizeof(pid_t));

    return srz_futex;
}

ARION_FUTEX *deserialize_arion_futex(std::vector<BYTE> srz_thread)
{
    ARION_FUTEX *arion_f = new ARION_FUTEX;

    off_t off = 0;
    memcpy(&arion_f->futex_addr, srz_thread.data() + off, sizeof(ADDR));
    off += sizeof(ADDR);
    memcpy(&arion_f->futex_bitmask, srz_thread.data() + off, sizeof(uint32_t));
    off += sizeof(uint32_t);
    memcpy(&arion_f->tid, srz_thread.data() + off, sizeof(pid_t));

    return arion_f;
}

std::vector<BYTE> serialize_arion_thread(ARION_THREAD *arion_t)
{
    std::vector<BYTE> srz_thread;

    srz_thread.insert(srz_thread.end(), (BYTE *)&arion_t->tid, (BYTE *)&arion_t->tid + sizeof(pid_t));
    srz_thread.insert(srz_thread.end(), (BYTE *)&arion_t->exit_signal, (BYTE *)&arion_t->exit_signal + sizeof(int));
    srz_thread.insert(srz_thread.end(), (BYTE *)&arion_t->flags, (BYTE *)&arion_t->flags + sizeof(uint64_t));
    srz_thread.insert(srz_thread.end(), (BYTE *)&arion_t->child_tid_addr,
                      (BYTE *)&arion_t->child_tid_addr + sizeof(ADDR));
    srz_thread.insert(srz_thread.end(), (BYTE *)&arion_t->parent_tid_addr,
                      (BYTE *)&arion_t->parent_tid_addr + sizeof(ADDR));
    if (arion_t->regs_state)
    {
        size_t regs_sz = arion_t->regs_state->size();
        srz_thread.insert(srz_thread.end(), (BYTE *)&regs_sz, (BYTE *)&regs_sz + sizeof(size_t));
        for (auto &reg : *arion_t->regs_state)
        {
            srz_thread.insert(srz_thread.end(), (BYTE *)&reg.first, (BYTE *)&reg.first + sizeof(REG));
            srz_thread.insert(srz_thread.end(), (BYTE *)&reg.second, (BYTE *)&reg.second + sizeof(RVAL));
        }
    }
    else
        srz_thread.insert(srz_thread.end(), (BYTE *)&EMPTY, (BYTE *)&EMPTY + sizeof(size_t));
    srz_thread.insert(srz_thread.end(), (BYTE *)&arion_t->lock, (BYTE *)&arion_t->lock + sizeof(int64_t));

    return srz_thread;
}

ARION_THREAD *deserialize_arion_thread(std::vector<BYTE> srz_thread)
{
    ARION_THREAD *arion_t = new ARION_THREAD;

    off_t off = 0;
    memcpy(&arion_t->tid, srz_thread.data() + off, sizeof(pid_t));
    off += sizeof(pid_t);
    memcpy(&arion_t->exit_signal, srz_thread.data() + off, sizeof(int));
    off += sizeof(int);
    memcpy(&arion_t->flags, srz_thread.data() + off, sizeof(uint64_t));
    off += sizeof(uint64_t);
    memcpy(&arion_t->child_tid_addr, srz_thread.data() + off, sizeof(ADDR));
    off += sizeof(ADDR);
    memcpy(&arion_t->parent_tid_addr, srz_thread.data() + off, sizeof(ADDR));
    off += sizeof(ADDR);
    size_t regs_sz;
    memcpy(&regs_sz, srz_thread.data() + off, sizeof(size_t));
    off += sizeof(ADDR);
    if (regs_sz)
    {
        std::unique_ptr<std::map<REG, RVAL>> regs_state = std::make_unique<std::map<REG, RVAL>>();
        off_t regs_end = off + regs_sz * (sizeof(REG) + sizeof(RVAL));
        while (off < regs_end)
        {
            REG reg;
            memcpy(&reg, srz_thread.data() + off, sizeof(REG));
            off += sizeof(REG);
            RVAL val;
            memcpy(&val, srz_thread.data() + off, sizeof(RVAL));
            off += sizeof(RVAL);
            regs_state->operator[](reg) = val;
        }
        arion_t->regs_state = std::move(regs_state);
    }
    memcpy(&arion_t->lock, srz_thread.data() + off, sizeof(int64_t));

    return arion_t;
}

std::map<pid_t, std::vector<std::unique_ptr<ARION_TGROUP_ENTRY>>> ThreadingManager::thread_groups;

std::map<int, std::string> ThreadingManager::sync_signals = {{SIGFPE, "SIGFPE (Floating-point exception)"},
                                                             {SIGILL, "SIGILL (Illegal instruction)"},
                                                             {SIGSEGV, "SIGSEGV (Segmentation fault)"},
                                                             {SIGBUS, "SIGBUS (Bus error)"},
                                                             {SIGTRAP, "SIGTRAP (Trace or breakpoint trap)"},
                                                             {SIGABRT, "SIGABRT (Abort)"},
                                                             {SIGSYS, "SIGSYS (Bad system call)"}};

std::unique_ptr<ThreadingManager> ThreadingManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::make_unique<ThreadingManager>(arion);
}

ThreadingManager::ThreadingManager(std::weak_ptr<Arion> arion) : arion(arion)
{
    std::shared_ptr<Arion> arion_ = arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");

    arion_->hooks->hook_intr(ThreadingManager::intr_hook);
    arion_->hooks->hook_mem_read_unmapped(ThreadingManager::invalid_memory_hook);
    arion_->hooks->hook_mem_write_unmapped(ThreadingManager::invalid_memory_hook);
    arion_->hooks->hook_mem_fetch_unmapped(ThreadingManager::invalid_memory_hook);
    arion_->hooks->hook_mem_read_prot(ThreadingManager::invalid_memory_hook);
    arion_->hooks->hook_mem_write_prot(ThreadingManager::invalid_memory_hook);
    arion_->hooks->hook_mem_fetch_prot(ThreadingManager::invalid_memory_hook);
    arion_->hooks->hook_insn_invalid(ThreadingManager::invalid_insn_hook);
}

ThreadingManager::~ThreadingManager()
{
    this->clear_threads();
}

void ThreadingManager::intr_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (arion->abi->has_idt_entry(intno))
    {
        CPU_INTR intr = arion->abi->get_idt_entry(intno);
        int signo = AbiManager::get_signal_from_intr(intr);
        arion->send_signal(arion->get_pid(), signo);
        arion->threads->handle_sync_signals();
    }
}

bool ThreadingManager::invalid_memory_hook(std::shared_ptr<Arion> arion, uc_mem_type access, uint64_t addr, int size,
                                           int64_t val, void *user_data)
{
    arion->send_signal(arion->get_pid(), SIGSEGV);
    arion->threads->handle_sync_signals();
    arion->sync_threads(); // Since Unicorn 2, returning true is not enough to gracefully recover from memory access
                           // error
    return true;
}

bool ThreadingManager::invalid_insn_hook(std::shared_ptr<Arion> arion, void *user_data)
{
    arion->send_signal(arion->get_pid(), SIGILL);
    arion->threads->handle_sync_signals();
    arion->sync_threads(); // Since Unicorn 2, returning true is not enough to gracefully recover from memory access
                           // error
    return true;
}

pid_t ThreadingManager::gen_next_id()
{
    pid_t tid;
    if (!this->free_thread_ids.empty())
    {
        tid = this->free_thread_ids.top();
        this->free_thread_ids.pop();
    }
    else
    {
        if (this->curr_id == ARION_MAX_U32)
            throw TooManyThreadsException();
        tid = this->curr_id++;
    }
    return tid;
}

pid_t ThreadingManager::add_thread_entry(std::unique_ptr<ARION_THREAD> thread)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    pid_t tid = this->gen_next_id();
    thread->tid = tid;
    if (!this->threads_map.size())
        this->running_tid = tid;
    this->threads_map[tid] = std::move(thread);
    this->set_tgid(tid, arion->get_pid(), true);
    return tid;
}

void ThreadingManager::remove_thread_entry(pid_t tid)
{
    if (this->threads_map.find(tid) == this->threads_map.end())
        throw WrongThreadIdException();

    if (this->threads_map.size() > 1)
        this->switch_to_next_thread();
    this->threads_map.erase(tid);
    if (this->threads_map.size())
        this->free_thread_ids.push(tid);
    else
    {
        this->free_thread_ids = std::stack<pid_t>();
        this->curr_id = 1;
    }
}

void ThreadingManager::clear_threads()
{
    std::vector<pid_t> thread_ids; // need to clone keys to prevent concurrency editing
    for (const auto &thread_pair : this->threads_map)
        thread_ids.push_back(thread_pair.first);
    for (HOOK_ID hook_id : thread_ids)
        this->remove_thread_entry(hook_id);
    this->futex_list.clear();
}

pid_t ThreadingManager::clone_thread(uint64_t flags, ADDR new_sp, ADDR new_tls, ADDR child_tid_addr,
                                     ADDR parent_tid_addr, int exit_signal)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    REG sp_reg = arion->abi->get_attrs()->regs.sp;
    REG tls_reg = arion->abi->get_attrs()->regs.tls;
    REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    ADDR pc = arion->abi->read_arch_reg(pc_reg);
    if (!new_sp)
        new_sp = arion->abi->read_arch_reg(sp_reg);
    ADDR next_pc;
    // For architectures that hook with hook_intr, next_pc is already returned
    if (arion->abi->does_hook_intr())
        next_pc = pc;
    else
    {
        size_t sys_instr_sz = arion->mem->read_instrs(pc, 1).at(0).size;
        next_pc = pc + sys_instr_sz;
    }
    CPU_ARCH arch = arion->abi->get_attrs()->arch;
    switch (arch)
    {
    case CPU_ARCH::X8664_ARCH: {
        if (!new_tls || !(flags & CLONE_SETTLS))
            new_tls = arion->abi->read_arch_reg(tls_reg);
        break;
    }
    case CPU_ARCH::X86_ARCH: {
        if (new_tls)
        {
            struct user_desc *u_desc = (struct user_desc *)malloc(sizeof(struct user_desc));
            std::vector<BYTE> u_desc_data = arion->mem->read(new_tls, sizeof(struct user_desc));
            memcpy(u_desc, u_desc_data.data(), u_desc_data.size());
            arion->gdt_manager->insert_entry(u_desc->entry_number, u_desc->base_addr, u_desc->limit,
                                             ARION_A_PRESENT | ARION_A_DATA | ARION_A_DATA_WRITABLE | ARION_A_PRIV_3 |
                                                 ARION_A_DIR_CON_BIT,
                                             ARION_F_PROT_32);
            free(u_desc);
        }
        break;
    }
    default:
        break;
    }
    std::unique_ptr<std::map<REG, RVAL>> regs = arion->abi->init_thread_regs(next_pc, new_sp, new_tls);
    regs->operator[](ret_reg).r64 = 0;
    std::unique_ptr<ARION_THREAD> arion_t =
        std::make_unique<ARION_THREAD>(exit_signal, flags, child_tid_addr, parent_tid_addr, std::move(regs));

    switch (arch)
    {
    case CPU_ARCH::ARM_ARCH: {
        if (new_tls)
        {
            uc_arm_cp_reg cp15 = {0};
            cp15.cp = 15;
            cp15.is64 = 0;
            cp15.sec = 0;
            cp15.crn = 13;
            cp15.crm = 0;
            cp15.opc1 = 0;
            cp15.opc2 = 3;
            cp15.val = new_tls;

            uc_err uc_reg_err = uc_reg_write(arion->uc, UC_ARM_REG_CP_REG, &cp15); // TPIDRURO
            if (uc_reg_err != UC_ERR_OK)
                throw UnicornRegWriteException(uc_reg_err);

            arion->mem->write_ptr(LINUX_32_ARM_GETTLS_ADDR + 0x10, new_tls);
        }
        break;
    }
    default:
        break;
    }

    pid_t parent_tid = arion->threads->get_running_tid();
    pid_t child_tid = arion->threads->add_thread_entry(std::move(arion_t));
    if (flags & CLONE_CHILD_SETTID)
        arion->mem->write_val(child_tid_addr, child_tid, sizeof(pid_t));
    if (flags & CLONE_PARENT_SETTID)
        arion->mem->write_val(parent_tid_addr, parent_tid, sizeof(pid_t));
    return child_tid;
}

void ThreadingManager::switch_to_thread(pid_t tid)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto curr_thread_it = this->threads_map.find(this->running_tid);
    if (curr_thread_it == this->threads_map.end())
        throw WrongThreadIdException();
    auto next_thread_it = this->threads_map.find(tid);
    if (next_thread_it == this->threads_map.end())
        throw WrongThreadIdException();
    std::unique_ptr<ARION_THREAD> curr_thread = std::move(curr_thread_it->second);
    std::unique_ptr<ARION_THREAD> next_thread = std::move(next_thread_it->second);

    curr_thread->regs_state = arion->abi->dump_regs();
    arion->abi->load_regs(std::move(next_thread->regs_state));

    this->threads_map[this->running_tid] = std::move(curr_thread);
    this->threads_map[tid] = std::move(next_thread);
    this->running_tid = tid;
}

void ThreadingManager::switch_to_next_thread()
{
    auto curr_thread_it = this->threads_map.find(this->running_tid);
    if (curr_thread_it == this->threads_map.end())
        throw WrongThreadIdException();
    auto next_thread_it = curr_thread_it;
    next_thread_it++;
    if (next_thread_it == this->threads_map.end())
        next_thread_it = this->threads_map.begin();
    if (next_thread_it == curr_thread_it)
        return;
    this->switch_to_thread(next_thread_it->first);
}

void ThreadingManager::futex_wait(pid_t tid, ADDR futex_addr, uint32_t futex_bitmask)
{
    std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(tid));
    std::unique_ptr<ARION_FUTEX> futex = std::make_unique<ARION_FUTEX>(futex_addr, futex_bitmask, tid);
    if (this->futex_list.find(futex_addr) == this->futex_list.end())
        this->futex_list[futex_addr] = new std::vector<std::unique_ptr<ARION_FUTEX>>();
    this->futex_list[futex_addr]->push_back(std::move(futex));
    arion_t->lock++;
    this->threads_map[tid] = std::move(arion_t);
}

void ThreadingManager::futex_wait_curr(ADDR futex_addr, uint32_t futex_bitmask)
{
    this->futex_wait(running_tid, futex_addr, futex_bitmask);
}

bool ThreadingManager::signal_wait(pid_t tid, pid_t target_pid)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (target_pid == arion->get_pid())
        throw WaitSameProcessException(target_pid);
    if (this->sigwait_list.find(tid) != this->sigwait_list.end())
        throw ThreadAlreadySigWaitingException(arion->get_pid(), tid);
    if (target_pid > 0 && !Arion::has_arion_instance(target_pid))
        return false;
    if (target_pid == 0 && !arion->get_pgid_children(arion->get_pgid()).size())
        return false;
    if (target_pid == -1 && !arion->get_children().size())
        return false;
    if (target_pid < -1 && !arion->get_pgid_children(-target_pid).size())
        return false;
    this->sigwait_list[tid] = target_pid;
    std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(tid));
    arion_t->lock++;
    this->threads_map[tid] = std::move(arion_t);
    return true;
}

bool ThreadingManager::signal_wait_curr(pid_t target_pid)
{
    return this->signal_wait(this->running_tid, target_pid);
}

bool ThreadingManager::has_sighandler(int signo)
{
    return this->sighandlers.find(signo) != this->sighandlers.end();
}

std::shared_ptr<struct ksigaction> ThreadingManager::get_sighandler(int signo)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto sighandler_it = this->sighandlers.find(signo);
    if (sighandler_it == this->sighandlers.end())
        throw NoSighandlerForSignalException(arion->get_pid(), this->running_tid, signo);
    return sighandler_it->second;
}

void ThreadingManager::set_sighandler(int signo, std::shared_ptr<struct ksigaction> sighandler)
{
    this->sighandlers[signo] = sighandler;
}

void ThreadingManager::handle_sync_signals()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    REG sp_reg = arion->abi->get_attrs()->regs.sp;
    std::vector<std::shared_ptr<SIGNAL>> new_pending_signals;
    for (std::shared_ptr<SIGNAL> sig : arion->pending_signals)
    {
        auto sync_sig_it = ThreadingManager::sync_signals.find(sig->signo);
        if (sync_sig_it == ThreadingManager::sync_signals.end())
        {
            new_pending_signals.push_back(sig);
            continue;
        }
        std::string sig_name = sync_sig_it->second;
        auto sighandler_it = this->sighandlers.find(sig->signo);
        if (sighandler_it == this->sighandlers.end())
            throw UnhandledSyncSignalException(arion->get_pid(), this->running_tid, sig_name);
        std::shared_ptr<struct ksigaction> handler = sighandler_it->second;
        std::shared_ptr<ARION_CONTEXT> ctxt = arion->context->save();
        ADDR curr_pc = arion->abi->read_arch_reg(pc_reg);
        arion->abi->write_arch_reg(pc_reg, (ADDR)handler->handler);
        // In both cases (with and without SA_SIGINFO), first parameter is signal number
        std::vector<REG> param_regs = arion->abi->get_attrs()->calling_conv.param_regs;
        arion->abi->write_arch_reg(param_regs.at(0), (uint64_t)sig->signo);
        if (handler->flags & SA_SIGINFO)
        {
            std::vector<REG> param_regs = arion->abi->get_attrs()->calling_conv.param_regs;
            arion->abi->write_arch_reg(param_regs.at(0), (uint64_t)sig->signo);
            std::unique_ptr<siginfo_t> info = std::make_unique<siginfo_t>();
            // TODO : Fill siginfo_t struct with missing fields
            info->si_signo = sig->signo;
            info->si_pid = sig->source_pid;
            uint64_t sp = arion->abi->read_arch_reg(sp_reg);
            sp -= sizeof(siginfo_t);
            arion->abi->write_arch_reg(sp_reg, sp);
            arion->mem->write(sp, (BYTE *)info.get(), sizeof(siginfo_t));
            arion->abi->write_arch_reg(param_regs.at(1), sp);
            arion->abi->write_arch_reg(param_regs.at(2), 0); // TODO : Provide ucontext here
        }
        arion->mem->stack_push(curr_pc);
        std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(this->running_tid));
        arion_t->lock = 0;
        this->threads_map[this->running_tid] = std::move(arion_t);

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
    }
    arion->pending_signals = new_pending_signals;
}

void ThreadingManager::handle_async_signals()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(this->running_tid));
    bool locked = arion_t->lock > 0;
    this->threads_map[this->running_tid] = std::move(arion_t);
    if (locked)
        return;
    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    REG sp_reg = arion->abi->get_attrs()->regs.sp;
    std::vector<std::shared_ptr<SIGNAL>> new_pending_signals;
    for (std::shared_ptr<SIGNAL> sig : arion->pending_signals)
    {
        if (ThreadingManager::sync_signals.find(sig->signo) != ThreadingManager::sync_signals.end())
        {
            new_pending_signals.push_back(sig);
            continue;
        }
        auto sighandler_it = this->sighandlers.find(sig->signo);
        if (sighandler_it == this->sighandlers.end())
        {
            std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(this->running_tid));
            if (arion_t->paused)
                arion_t->paused = false;
            else
                new_pending_signals.push_back(sig);
            this->threads_map[this->running_tid] = std::move(arion_t);
            continue;
        }
        std::shared_ptr<struct ksigaction> handler = sighandler_it->second;
        std::shared_ptr<ARION_CONTEXT> ctxt = arion->context->save();
        ADDR curr_pc = arion->abi->read_arch_reg(pc_reg);
        arion->abi->write_arch_reg(pc_reg, (ADDR)handler->handler);
        // In both cases (with and without SA_SIGINFO), first parameter is signal number
        std::vector<REG> param_regs = arion->abi->get_attrs()->calling_conv.param_regs;
        arion->abi->write_arch_reg(param_regs.at(0), (uint64_t)sig->signo);
        if (handler->flags & SA_SIGINFO)
        {
            std::unique_ptr<siginfo_t> info = std::make_unique<siginfo_t>();
            memset(info.get(), 0, sizeof(siginfo_t));
            // TODO : Fill siginfo_t struct with missing fields
            info->si_signo = sig->signo;
            info->si_pid = sig->source_pid;
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
    }
    arion->pending_signals = new_pending_signals;
}

void ThreadingManager::handle_wait_signals()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto wait_it = this->sigwait_list.find(this->running_tid);
    if (wait_it == this->sigwait_list.end())
        return;
    pid_t target_pid = wait_it->second;
    std::vector<std::shared_ptr<SIGNAL>> new_pending_signals;
    bool found_sig = false;
    REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    for (std::shared_ptr<SIGNAL> sig : arion->pending_signals)
    {
        if (found_sig)
        {
            new_pending_signals.push_back(sig);
            continue;
        }
        std::weak_ptr<Arion> source_instance_weak = Arion::get_arion_instance(sig->source_pid);
        std::shared_ptr<Arion> source_instance = source_instance_weak.lock();
        if (!arion)
            throw ExpiredWeakPtrException("Arion");
        if (target_pid == sig->source_pid ||
            (target_pid == 0 && arion->has_child(sig->source_pid) &&
             arion->get_pgid() == source_instance->get_pgid()) ||
            (target_pid == -1 && arion->has_child(sig->source_pid)) ||
            (target_pid < -1 && arion->has_child(sig->source_pid) && source_instance->get_pgid() == -target_pid))
        {
            found_sig = true;
            std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(running_tid));
            arion_t->lock--;
            if (arion_t->wait_status_addr)
            {
                arion->mem->write_val(arion_t->wait_status_addr, 0, sizeof(int));
                arion_t->wait_status_addr = 0;
            }
            arion->abi->write_arch_reg(ret_reg, sig->source_pid);
            this->threads_map[running_tid] = std::move(arion_t);
            if (sig->signo == SIGCHLD)
            {
                source_instance->cleanup_process();
                arion->remove_child(sig->source_pid);
            }
        }
        else
            new_pending_signals.push_back(sig);
    }
    arion->pending_signals = new_pending_signals;
    if (found_sig)
        this->sigwait_list.erase(this->running_tid);
}

void ThreadingManager::handle_signals()
{
    this->handle_sync_signals();
    this->handle_async_signals();
    this->handle_wait_signals();
}

size_t ThreadingManager::futex_wake(ADDR futex_addr, uint32_t futex_bitmask)
{
    size_t awaken_count = 0;
    if (this->futex_list.find(futex_addr) == this->futex_list.end())
        return awaken_count;
    std::map<ADDR, std::vector<std::unique_ptr<ARION_FUTEX>> *>
        futex_list; // need to clone to prevent concurrency editing
    std::vector<std::unique_ptr<ARION_FUTEX>> *addr_futex = new std::vector<std::unique_ptr<ARION_FUTEX>>();
    for (std::unique_ptr<ARION_FUTEX> &futex : *this->futex_list[futex_addr])
    {
        if (futex->futex_bitmask & futex_bitmask)
        {
            std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(futex->tid));
            arion_t->lock--;
            this->threads_map[futex->tid] = std::move(arion_t);
            awaken_count++;
        }
        else
            addr_futex->push_back(std::move(futex));
    }
    if (addr_futex->size())
        this->futex_list[futex_addr] = addr_futex;
    else
        this->futex_list.erase(futex_addr);
    return awaken_count;
}

bool ThreadingManager::is_curr_locked()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (arion->is_zombie)
        return true;
    std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(running_tid));
    bool curr_locked = arion_t->paused || arion_t->lock > 0;
    this->threads_map[running_tid] = std::move(arion_t);
    return curr_locked;
}

size_t ThreadingManager::get_threads_count()
{
    return this->threads_map.size();
}

pid_t ThreadingManager::get_running_tid()
{
    return this->running_tid;
}

void ThreadingManager::set_running_tid(pid_t tid)
{
    this->running_tid = tid;
}

void ThreadingManager::set_tgid(pid_t tid, pid_t tgid, bool init)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (this->threads_map.find(tid) == this->threads_map.end())
        throw WrongThreadIdException();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(this->threads_map.at(tid));
    pid_t old_tgid = arion_t->tgid;
    arion_t->tgid = tgid;
    this->threads_map[tid] = std::move(arion_t);
    if (!init)
    {
        auto old_tgid_it = ThreadingManager::thread_groups.find(old_tgid);
        if (old_tgid_it != ThreadingManager::thread_groups.end())
        {
            std::vector<std::unique_ptr<ARION_TGROUP_ENTRY>> &old_tgid_vec = old_tgid_it->second;
            auto old_tid_it =
                std::find_if(old_tgid_vec.begin(), old_tgid_vec.end(),
                             [tid](std::unique_ptr<ARION_TGROUP_ENTRY> &entry) { return entry->tid = tid; });
            if (old_tid_it != old_tgid_vec.end())
                old_tgid_vec.erase(old_tid_it);
        }
    }
    std::vector<std::unique_ptr<ARION_TGROUP_ENTRY>> new_tgid_vec;
    auto new_tgid_it = ThreadingManager::thread_groups.find(tgid);
    if (new_tgid_it != ThreadingManager::thread_groups.end())
        new_tgid_vec = std::move(new_tgid_it->second);
    else
        new_tgid_vec = std::vector<std::unique_ptr<ARION_TGROUP_ENTRY>>();
    std::unique_ptr<ARION_TGROUP_ENTRY> entry = std::make_unique<ARION_TGROUP_ENTRY>(tid, tgid, arion->get_pid());
    new_tgid_vec.push_back(std::move(entry));
    ThreadingManager::thread_groups[tgid] = std::move(new_tgid_vec);
}
