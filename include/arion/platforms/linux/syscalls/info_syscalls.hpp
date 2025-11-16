#ifndef ARION_INFO_SYSCALLS_HPP
#define ARION_INFO_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

/**
 * Emulates the Linux `newuname()` syscall, retrieving system information.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (struct `newutsname` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_newuname(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `gettimeofday()` syscall, getting the current time and timezone information.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (`timeval` pointer, `timezone` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_gettimeofday(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getrlimit()` syscall, getting resource limits.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (resource, `rlimit` structure pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_getrlimit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `sysinfo()` syscall, retrieving overall system statistics.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (`sysinfo` structure pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_sysinfo(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `capget()` syscall, getting process capabilities.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (`header` pointer, `data` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_capget(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `capset()` syscall, setting process capabilities.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (`header` pointer, `data` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_capset(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `arch_prctl()` syscall, for architecture-specific process control operations (primarily x86-64
 * thread segment bases).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (function code, address/value).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_arch_prctl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `prlimit64()` syscall, getting/setting resource limits for an arbitrary process.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid, resource, `new_rlimit` pointer, `old_rlimit` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_prlimit64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getcpu()` syscall, determining the current CPU and NUMA node.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (cpu_ptr, node_ptr, tcache_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_getcpu(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

} // namespace arion

#endif // ARION_INFO_SYSCALLS_HPP
