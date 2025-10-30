#ifndef ARION_INFO_SYSCALLS_HPP
#define ARION_INFO_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

uint64_t sys_newuname(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_gettimeofday(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_getrlimit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_sysinfo(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_capget(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_capset(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_arch_prctl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_prlimit64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
uint64_t sys_getcpu(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

} // namespace arion

#endif // ARION_INFO_SYSCALLS_HPP
