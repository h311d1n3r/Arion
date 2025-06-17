#ifndef ARION_PROCESS_SYSCALLS_HPP
#define ARION_PROCESS_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

uint64_t sys_clone(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_fork(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_execve(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_exit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_futex(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_futex_time64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_set_tid_address(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_set_thread_area(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_set_tls(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_exit_group(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_clone3(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_set_robust_list(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_rseq(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);

#endif // ARION_PROCESS_SYSCALLS_HPP
