#ifndef ARION_ID_SYSCALLS_HPP
#define ARION_ID_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

uint64_t sys_getpid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_geteuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getegid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setpgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getppid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getpgrp(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setsid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setreuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setregid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getgroups(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setgroups(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setresuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getresuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setresgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getresgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getpgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setfsuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setfsgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getsid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_gettid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);

#endif // ARION_ID_SYSCALLS_HPP
