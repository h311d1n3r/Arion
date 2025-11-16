#ifndef ARION_MISC_SYSCALLS_HPP
#define ARION_MISC_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

namespace arion
{

/**
 * Emulates the Linux `getrandom()` syscall, filling a buffer with random bytes.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (buf_ptr, buflen, flags).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The number of bytes stored in the buffer on success, or -1 on error.
 */
uint64_t sys_getrandom(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

}; // namespace arion

#endif // ARION_MISC_SYSCALLS_HPP
