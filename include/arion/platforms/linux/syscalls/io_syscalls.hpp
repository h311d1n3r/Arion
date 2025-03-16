#ifndef ARION_IO_SYSCALLS_HPP
#define ARION_IO_SYSCALLS_HPP

#include <arion/platforms/linux/lnx_syscall_manager.hpp>

uint64_t sys_read(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_write(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_open(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_close(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_newstat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_newfstat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_newlstat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_poll(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_lseek(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_ioctl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_pread64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_pwrite64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_readv(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_writev(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_access(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_select(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_dup(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_dup2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_socket(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_socketcall(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_connect(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_accept(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_sendto(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_send(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_recvfrom(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_recv(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_sendmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_recvmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_shutdown(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_bind(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_listen(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getsockname(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getpeername(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_socketpair(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_setsockopt(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getsockopt(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_fcntl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_truncate(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_ftruncate(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_faccessat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getcwd(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_chdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_fchdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_rename(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_mkdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_rmdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_creat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_link(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_unlink(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_symlink(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_readlink(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_statfs(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_fstatfs(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_getdents64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_openat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_newfstatat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_renameat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_readlinkat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_pselect6(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_ppoll(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_accept4(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_recvmmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_sendmmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_renameat2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);
uint64_t sys_statx(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params);

#endif // ARION_IO_SYSCALLS_HPP
