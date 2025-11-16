#ifndef ARION_SIGNAL_SYSCALLS_HPP
#define ARION_SIGNAL_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

/**
 * Emulates the Linux `rt_sigaction()` syscall, allowing examination and change of a signal action (real-time safe
 * version).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (signum, act_ptr, oldact_ptr, sigsetsize).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_rt_sigaction(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `rt_sigreturn()` syscall, returning from a signal handler (real-time safe version).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return Does not return on success; returns -1 on error if it attempts to return.
 */
uint64_t sys_rt_sigreturn(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `sigreturn()` syscall, returning from a signal handler (legacy version).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return Does not return on success; returns -1 on error if it attempts to return.
 */
uint64_t sys_sigreturn(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `pause()` syscall, suspending the process until a signal is received.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return Always returns -1 with `EINTR` (interrupted system call).
 */
uint64_t sys_pause(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `wait4()` syscall, waiting for process termination, reporting resource usage.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid, wstatus_ptr, options, rusage_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The PID of the terminated child, 0 if non-blocking and no child has exited, or -1 on error.
 */
uint64_t sys_wait4(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `kill()` syscall, sending a signal to a process or process group.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid, sig).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_kill(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `tgkill()` syscall, sending a signal to a specific thread within a process group.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (tgid, tid, sig).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_tgkill(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `waitid()` syscall, waiting for a state change in a child process.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (idtype, id, infop_ptr, options, rusage_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_waitid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

}; // namespace arion

#endif // ARION_SIGNAL_SYSCALLS_HPP
