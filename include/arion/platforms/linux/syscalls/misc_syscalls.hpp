#ifndef ARION_MISC_SYSCALLS_HPP
#define ARION_MISC_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

uint64_t sys_getrandom(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);

#endif // ARION_MISC_SYSCALLS_HPP
