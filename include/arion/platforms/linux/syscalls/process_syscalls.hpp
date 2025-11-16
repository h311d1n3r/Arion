#ifndef ARION_PROCESS_SYSCALLS_HPP
#define ARION_PROCESS_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

/**
 * Emulates the Linux `clone()` syscall, creating a child process/thread with fine-grained control over what is shared.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (flags, child_stack_ptr, ptid_ptr, ctid_ptr, new_tls).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The thread ID (TID) of the child on success in the parent, 0 in the child, or -1 on error.
 */
uint64_t sys_clone(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `fork()` syscall, creating a child process that is a near duplicate of the parent.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The process ID (PID) of the child on success in the parent, 0 in the child, or -1 on error.
 */
uint64_t sys_fork(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `execve()` syscall, executing a program.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pathname_ptr, argv_ptr, envp_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return Does not return on success; returns -1 on error.
 */
uint64_t sys_execve(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `exit()` syscall, terminating the calling thread/process with a specified exit status.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (exit_code).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return Does not return.
 */
uint64_t sys_exit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `futex()` syscall, a fast user-space locking mechanism.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (uaddr, op, val, timeout_ptr, uaddr2, val3).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return A non-negative value on success (meaning depends on op), or -1 on error.
 */
uint64_t sys_futex(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `futex_time64()` syscall, a fast user-space locking mechanism with 64-bit timeout parameter (for
 * compatibility).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (uaddr, op, val, timeout_ptr, uaddr2, val3).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return A non-negative value on success (meaning depends on op), or -1 on error.
 */
uint64_t sys_futex_time64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `set_tid_address()` syscall, setting a pointer to the thread ID for the futex mechanism.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (tidptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The thread ID (TID) of the calling thread.
 */
uint64_t sys_set_tid_address(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `set_thread_area()` syscall, setting a Thread Local Storage (TLS) segment register for the calling
 * thread (legacy x86).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (user_desc_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_set_thread_area(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `set_tls()` syscall, setting the Thread Local Storage (TLS) for the calling thread (modern
 * architecture version).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (tls_val).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_set_tls(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `exit_group()` syscall, terminating all threads in the process with a specified exit status.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (exit_code).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return Does not return.
 */
uint64_t sys_exit_group(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `clone3()` syscall, creating a child process/thread with an extended argument structure (new
 * modern clone).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (cl_args_ptr, size).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The PID/TID of the child on success in the parent, 0 in the child, or -1 on error.
 */
uint64_t sys_clone3(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `set_robust_list()` syscall, setting the head of the thread's robust futex list.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (head_ptr, len).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_set_robust_list(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `rseq()` syscall, registering a Restartable Sequence for fast user-space synchronization.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (rseq_addr, rseq_len, flags, sig).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_rseq(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

}; // namespace arion

#endif // ARION_PROCESS_SYSCALLS_HPP
