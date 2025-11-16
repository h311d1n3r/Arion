#ifndef ARION_MEM_SYSCALLS_HPP
#define ARION_MEM_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

#define ARION_MMAP_MIN_ADDR 0x10000

namespace arion
{

/**
 * Emulates the Linux `mmap()` syscall, mapping files or devices into memory.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (addr, len, prot, flags, fd, offset).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The address of the mapped area on success, or `MAP_FAILED` (-1) on error.
 */
uint64_t sys_mmap(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `mmap2()` syscall, mapping files or devices into memory (32-bit Linux optimized version using
 * page-sized offset).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (addr, len, prot, flags, fd, pgoffset).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The address of the mapped area on success, or `MAP_FAILED` (-1) on error.
 */
uint64_t sys_mmap2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `mprotect()` syscall, setting protection on a region of memory.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (addr, len, prot).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_mprotect(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `munmap()` syscall, unmapping files or devices from memory.
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (addr, len).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return 0 on success, or -1 on error.
 */
uint64_t sys_munmap(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);
/**
 * Emulates the Linux `brk()` syscall, changing the size of the data segment (program break).
 * @param[in] arion Shared pointer to the main Arion instance.
 * @param[in] params Vector of system call parameters (addr).
 * @param[out] cancel **(Output)** If set to `true`, the syscall execution is deferred, the PC is rewound, and the
 * return value is ignored.
 * @return The new program break address on success, or the current break address on failure.
 */
uint64_t sys_brk(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel);

} // namespace arion

#endif // ARION_MEM_SYSCALLS_HPP
