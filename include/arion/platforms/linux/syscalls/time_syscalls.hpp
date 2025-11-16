#ifndef ARION_TIME_SYSCALLS_HPP
#define ARION_TIME_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

/**
 * Emulates the Linux `sched_rr_get_interval()` syscall, retrieving the time slice allocated to a round-robin thread.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid, `timespec` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_sched_rr_get_interval(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `time()` syscall, retrieving the current time as seconds since the Epoch.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (`time_t` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The current time on success, or -1 on error.
 */
uint64_t sys_time(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `clock_gettime()` syscall, getting the time of a specified clock.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (clockid, `timespec` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_clock_gettime(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `clock_getres()` syscall, getting the resolution of a specified clock.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (clockid, `timespec` pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_clock_getres(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `clock_nanosleep()` syscall, suspending execution for an interval with respect to a clock.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (clockid, flags, `request` timespec pointer, `remain` timespec
 * pointer).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or an error number on failure.
 */
uint64_t sys_clock_nanosleep(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

}; // namespace arion

#endif // ARION_TIME_SYSCALLS_HPP
