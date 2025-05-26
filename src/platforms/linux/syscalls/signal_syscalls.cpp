#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/threading_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <arion/platforms/linux/syscalls/signal_syscalls.hpp>
#include <memory>
#include <sys/wait.h>

using namespace arion;

uint64_t sys_rt_sigaction(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int signo = params.at(0);
    ADDR act_addr = params.at(1);
    ADDR old_act_addr = params.at(2);
    size_t sig_set_sz = params.at(3);

    if (old_act_addr && arion->signals->has_sighandler(signo))
    {
        std::shared_ptr<struct ksigaction> old_act = arion->signals->get_sighandler(signo);
        std::vector<BYTE> old_act_data(sizeof(struct ksigaction));
        memcpy(old_act_data.data(), old_act.get(), sizeof(struct ksigaction));
        arion->mem->write(old_act_addr, old_act_data.data(), old_act_data.size());
    }
    if (act_addr)
    {
        std::vector<BYTE> act_data = arion->mem->read(act_addr, sizeof(struct ksigaction));
        std::shared_ptr<struct ksigaction> act = std::make_shared<struct ksigaction>();
        memcpy(act.get(), act_data.data(), act_data.size());
        arion->signals->set_sighandler(signo, act);
    }
    return 0;
}

uint64_t sys_rt_sigreturn(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    if (arion->signals->sigreturn())
    {
        REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
        RVAL64 ret = arion->abi->read_arch_reg(ret_reg);
        return ret;
    }
    return -1;
}

uint64_t sys_sigreturn(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    if (arion->signals->sigreturn())
    {
        REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
        RVAL64 ret = arion->abi->read_arch_reg(ret_reg);
        return ret;
    }
    return -1;
}

uint64_t sys_pause(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    arion_t->stopped = true;
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    return 0;
}

uint64_t sys_wait4(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t pid = params.at(0);
    ADDR stat_addr = params.at(1);
    int options = params.at(2);
    ADDR rusage_addr = params.at(3);

    pid_t curr_tid = arion->threads->get_running_tid();
    bool waiting = arion->threads->signal_wait_curr(pid, stat_addr);
    arion->sync_threads();
    return 0;
}

uint64_t sys_kill(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t pid = params.at(0);
    int sig = params.at(1);

    std::shared_ptr<Arion> target = arion->get_group()->get_arion_instance(pid);

    target->send_signal(arion->get_pid(), sig);
    return 0;
}

uint64_t sys_tgkill(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t tgid = params.at(0);
    pid_t tid = params.at(1);
    int sig = params.at(2);

    if (tgid <= 0 || tid <= 0)
        return EINVAL;
    auto tg_it = arion->threads->thread_groups.find(tgid);
    if (tg_it == arion->threads->thread_groups.end())
        return EINVAL;
    auto entry_it = std::find_if(tg_it->second.begin(), tg_it->second.end(),
                                 [tid](std::unique_ptr<ARION_TGROUP_ENTRY> &entry) { return entry->tid == tid; });
    if (entry_it == tg_it->second.end())
        return EINVAL;

    std::shared_ptr<Arion> target = arion->get_group()->get_arion_instance((*entry_it)->pid);

    target->send_signal(arion->get_pid(), sig);
    arion->sync_threads();
    return 0;
}

uint64_t sys_waitid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int idtype = params.at(0);
    id_t id = params.at(1);
    ADDR siginfo_addr = params.at(2);
    int options = params.at(3);
    ADDR rusage_addr = params.at(4);

    pid_t wait_pid = 0; // PID value based on wait4 syscall
    switch (idtype)
    {
    case P_PID:
        wait_pid = id;
        break;
    case P_PGID:
        if (id)
            wait_pid = -id;
        else
            wait_pid = 0;
        break;
        wait_pid = -1;
    }
    arion->threads->signal_wait_curr(wait_pid, 0);
    arion->sync_threads();
    return 0;
}
