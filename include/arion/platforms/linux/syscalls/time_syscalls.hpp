#ifndef ARION_TIME_SYSCALLS_HPP
#define ARION_TIME_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

uint64_t sys_sched_rr_get_interval(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params);
uint64_t sys_time(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params);
uint64_t sys_clock_gettime(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params);
uint64_t sys_clock_getres(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params);
uint64_t sys_clock_nanosleep(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params);

#endif // ARION_TIME_SYSCALLS_HPP
