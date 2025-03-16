#ifndef ARION_MEM_SYSCALLS_HPP
#define ARION_MEM_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

#define MMAP_MIN_ADDR 0x10000

uint64_t sys_mmap(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_mmap2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_mprotect(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_munmap(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_brk(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);

#endif // ARION_MEM_SYSCALLS_HPP
