#ifndef ARION_ID_SYSCALLS_HPP
#define ARION_ID_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

/**
 * Emulates the Linux `getpid()` syscall, retrieving the process ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The process ID (PID) of the calling process.
 */
uint64_t sys_getpid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getuid()` syscall, retrieving the real user ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The real user ID (UID) of the calling process.
 */
uint64_t sys_getuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getgid()` syscall, retrieving the real group ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The real group ID (GID) of the calling process.
 */
uint64_t sys_getgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setuid()` syscall, setting the effective user ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (new UID).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setgid()` syscall, setting the effective group ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (new GID).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `geteuid()` syscall, retrieving the effective user ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The effective user ID (EUID) of the calling process.
 */
uint64_t sys_geteuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getegid()` syscall, retrieving the effective group ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The effective group ID (EGID) of the calling process.
 */
uint64_t sys_getegid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setpgid()` syscall, setting the process group ID of a process.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid, pgid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setpgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getppid()` syscall, retrieving the parent process ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The parent process ID (PPID) of the calling process.
 */
uint64_t sys_getppid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getpgrp()` syscall, retrieving the process group ID of the calling process.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The process group ID (PGRP) of the calling process.
 */
uint64_t sys_getpgrp(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setsid()` syscall, creating a new session and setting the process group ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The new session ID (SID) on success, or -1 on error.
 */
uint64_t sys_setsid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setreuid()` syscall, setting the real and effective user IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (real_uid, effective_uid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setreuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setregid()` syscall, setting the real and effective group IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (real_gid, effective_gid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setregid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getgroups()` syscall, getting the list of supplementary group IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (size, list_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The number of supplementary group IDs on success, or -1 on error.
 */
uint64_t sys_getgroups(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setgroups()` syscall, setting the list of supplementary group IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (size, list_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setgroups(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setresuid()` syscall, setting the real, effective, and saved set-user IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (real_uid, effective_uid, saved_uid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setresuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getresuid()` syscall, retrieving the real, effective, and saved set-user IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (real_uid_ptr, effective_uid_ptr, saved_uid_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_getresuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setresgid()` syscall, setting the real, effective, and saved set-group IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (real_gid, effective_gid, saved_gid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_setresgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getresgid()` syscall, retrieving the real, effective, and saved set-group IDs.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (real_gid_ptr, effective_gid_ptr, saved_gid_ptr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_getresgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getpgid()` syscall, retrieving the process group ID for a specific process ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The process group ID on success, or -1 on error.
 */
uint64_t sys_getpgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setfsuid()` syscall, setting the File System User ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (fsuid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The previous File System User ID.
 */
uint64_t sys_setfsuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `setfsgid()` syscall, setting the File System Group ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (fsgid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The previous File System Group ID.
 */
uint64_t sys_setfsgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `getsid()` syscall, retrieving the session ID for a specific process ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (pid).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The session ID on success, or -1 on error.
 */
uint64_t sys_getsid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `gettid()` syscall, retrieving the thread ID.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters.
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The thread ID (TID) of the calling thread.
 */
uint64_t sys_gettid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

}; // namespace arion

#endif // ARION_ID_SYSCALLS_HPP
