#ifndef ARION_SIGNAL_SYSCALLS_HPP
#define ARION_SIGNAL_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

uint64_t sys_rt_sigaction(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_pause(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_wait4(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_kill(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_tgkill(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_waitid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);

#endif // ARION_SIGNAL_SYSCALLS_HPP
