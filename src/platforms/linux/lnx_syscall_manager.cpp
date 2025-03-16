#include "arion/utils/convert_utils.hpp"
#include <arion/arion.hpp>
#include <arion/platforms/linux/lnx_syscall_manager.hpp>
#include <arion/platforms/linux/syscalls/id_syscalls.hpp>
#include <arion/platforms/linux/syscalls/info_syscalls.hpp>
#include <arion/platforms/linux/syscalls/io_syscalls.hpp>
#include <arion/platforms/linux/syscalls/mem_syscalls.hpp>
#include <arion/platforms/linux/syscalls/misc_syscalls.hpp>
#include <arion/platforms/linux/syscalls/process_syscalls.hpp>
#include <arion/platforms/linux/syscalls/signal_syscalls.hpp>
#include <arion/platforms/linux/syscalls/time_syscalls.hpp>

using namespace arion;

std::map<std::string, uint8_t> PARAMS_N_BY_SYSCALL_NAME = {{"read", 3},
                                                           {"write", 3},
                                                           {"open", 3},
                                                           {"close", 1},
                                                           {"newstat", 2},
                                                           {"newfstat", 2},
                                                           {"newlstat", 2},
                                                           {"poll", 3},
                                                           {"lseek", 3},
                                                           {"mmap", 6},
                                                           {"mmap2", 6},
                                                           {"mmap_pgoff", 6},
                                                           {"mprotect", 3},
                                                           {"munmap", 2},
                                                           {"brk", 1},
                                                           {"rt_sigaction", 4},
                                                           {"rt_sigprocmask", 4},
                                                           {"rt_sigreturn", 0},
                                                           {"ioctl", 3},
                                                           {"pread64", 4},
                                                           {"pwrite64", 4},
                                                           {"readv", 3},
                                                           {"writev", 3},
                                                           {"access", 2},
                                                           {"pipe", 1},
                                                           {"select", 5},
                                                           {"sched_yield", 0},
                                                           {"mremap", 5},
                                                           {"msync", 3},
                                                           {"mincore", 3},
                                                           {"madvise", 3},
                                                           {"shmget", 3},
                                                           {"shmat", 3},
                                                           {"shmctl", 3},
                                                           {"dup", 1},
                                                           {"dup2", 2},
                                                           {"pause", 0},
                                                           {"nanosleep", 2},
                                                           {"getitimer", 2},
                                                           {"alarm", 1},
                                                           {"setitimer", 3},
                                                           {"getpid", 0},
                                                           {"sendfile64", 4},
                                                           {"socket", 3},
                                                           {"socketcall", 2},
                                                           {"connect", 3},
                                                           {"accept", 3},
                                                           {"sendto", 6},
                                                           {"send", 4},
                                                           {"recvfrom", 6},
                                                           {"recv", 4},
                                                           {"sendmsg", 3},
                                                           {"recvmsg", 3},
                                                           {"shutdown", 2},
                                                           {"bind", 3},
                                                           {"listen", 2},
                                                           {"getsockname", 3},
                                                           {"getpeername", 3},
                                                           {"socketpair", 4},
                                                           {"setsockopt", 5},
                                                           {"getsockopt", 5},
                                                           {"clone", 5},
                                                           {"fork", 0},
                                                           {"vfork", 0},
                                                           {"execve", 3},
                                                           {"exit", 1},
                                                           {"wait4", 4},
                                                           {"kill", 2},
                                                           {"newuname", 1},
                                                           {"semget", 3},
                                                           {"semop", 3},
                                                           {"semctl", 4},
                                                           {"shmdt", 1},
                                                           {"msgget", 2},
                                                           {"msgsnd", 4},
                                                           {"msgrcv", 5},
                                                           {"msgctl", 3},
                                                           {"fcntl", 3},
                                                           {"flock", 2},
                                                           {"fsync", 1},
                                                           {"fdatasync", 1},
                                                           {"truncate", 2},
                                                           {"ftruncate", 2},
                                                           {"getdents", 3},
                                                           {"getcwd", 2},
                                                           {"chdir", 1},
                                                           {"fchdir", 1},
                                                           {"rename", 2},
                                                           {"mkdir", 2},
                                                           {"rmdir", 1},
                                                           {"creat", 2},
                                                           {"link", 2},
                                                           {"unlink", 1},
                                                           {"symlink", 2},
                                                           {"readlink", 3},
                                                           {"chmod", 2},
                                                           {"fchmod", 2},
                                                           {"chown", 3},
                                                           {"fchown", 3},
                                                           {"lchown", 3},
                                                           {"umask", 1},
                                                           {"gettimeofday", 2},
                                                           {"getrlimit", 2},
                                                           {"old_getrlimit", 2},
                                                           {"getrusage", 2},
                                                           {"sysinfo", 1},
                                                           {"times", 1},
                                                           {"ptrace", 4},
                                                           {"getuid", 0},
                                                           {"syslog", 3},
                                                           {"getgid", 0},
                                                           {"setuid", 1},
                                                           {"setgid", 1},
                                                           {"geteuid", 0},
                                                           {"getegid", 0},
                                                           {"setpgid", 2},
                                                           {"getppid", 0},
                                                           {"getpgrp", 0},
                                                           {"setsid", 0},
                                                           {"setreuid", 2},
                                                           {"setregid", 2},
                                                           {"getgroups", 2},
                                                           {"setgroups", 2},
                                                           {"setresuid", 3},
                                                           {"getresuid", 3},
                                                           {"setresgid", 3},
                                                           {"getresgid", 3},
                                                           {"getpgid", 1},
                                                           {"setfsuid", 1},
                                                           {"setfsgid", 1},
                                                           {"getsid", 1},
                                                           {"capget", 2},
                                                           {"capset", 2},
                                                           {"rt_sigpending", 2},
                                                           {"rt_sigtimedwait", 4},
                                                           {"rt_sigqueueinfo", 3},
                                                           {"rt_sigsuspend", 2},
                                                           {"sigaltstack", 2},
                                                           {"utime", 2},
                                                           {"mknod", 3},
                                                           {"personality", 1},
                                                           {"ustat", 2},
                                                           {"statfs", 2},
                                                           {"fstatfs", 2},
                                                           {"sysfs", 3},
                                                           {"getpriority", 2},
                                                           {"setpriority", 3},
                                                           {"sched_setparam", 2},
                                                           {"sched_getparam", 2},
                                                           {"sched_setscheduler", 3},
                                                           {"sched_getscheduler", 1},
                                                           {"sched_get_priority_max", 1},
                                                           {"sched_get_priority_min", 1},
                                                           {"sched_rr_get_interval", 2},
                                                           {"mlock", 2},
                                                           {"munlock", 2},
                                                           {"mlockall", 1},
                                                           {"munlockall", 0},
                                                           {"vhangup", 0},
                                                           {"modify_ldt", 3},
                                                           {"pivot_root", 2},
                                                           {"prctl", 5},
                                                           {"arch_prctl", 2},
                                                           {"adjtimex", 1},
                                                           {"setrlimit", 2},
                                                           {"chroot", 1},
                                                           {"sync", 0},
                                                           {"acct", 1},
                                                           {"settimeofday", 2},
                                                           {"mount", 5},
                                                           {"umount", 2},
                                                           {"swapon", 2},
                                                           {"swapoff", 1},
                                                           {"reboot", 4},
                                                           {"sethostname", 2},
                                                           {"setdomainname", 2},
                                                           {"iopl", 1},
                                                           {"ioperm", 3},
                                                           {"init_module", 3},
                                                           {"delete_module", 2},
                                                           {"quotactl", 4},
                                                           {"gettid", 0},
                                                           {"readahead", 3},
                                                           {"setxattr", 5},
                                                           {"lsetxattr", 5},
                                                           {"fsetxattr", 5},
                                                           {"getxattr", 4},
                                                           {"lgetxattr", 4},
                                                           {"fgetxattr", 4},
                                                           {"listxattr", 3},
                                                           {"llistxattr", 3},
                                                           {"flistxattr", 3},
                                                           {"removexattr", 2},
                                                           {"lremovexattr", 2},
                                                           {"fremovexattr", 2},
                                                           {"tkill", 2},
                                                           {"time", 1},
                                                           {"futex", 6},
                                                           {"sched_setaffinity", 3},
                                                           {"sched_getaffinity", 3},
                                                           {"io_setup", 2},
                                                           {"io_destroy", 1},
                                                           {"io_getevents", 5},
                                                           {"io_submit", 3},
                                                           {"io_cancel", 3},
                                                           {"epoll_create", 1},
                                                           {"remap_file_pages", 5},
                                                           {"getdents64", 3},
                                                           {"set_tid_address", 1},
                                                           {"set_thread_area", 1},
                                                           {"set_tls", 1},
                                                           {"get_thread_area", 1},
                                                           {"restart_syscall", 0},
                                                           {"semtimedop", 4},
                                                           {"fadvise64", 4},
                                                           {"timer_create", 3},
                                                           {"timer_settime", 4},
                                                           {"timer_gettime", 2},
                                                           {"timer_getoverrun", 1},
                                                           {"timer_delete", 1},
                                                           {"clock_settime", 2},
                                                           {"clock_gettime", 2},
                                                           {"clock_getres", 2},
                                                           {"clock_nanosleep", 4},
                                                           {"exit_group", 1},
                                                           {"epoll_wait", 4},
                                                           {"epoll_ctl", 4},
                                                           {"tgkill", 3},
                                                           {"utimes", 2},
                                                           {"mbind", 6},
                                                           {"set_mempolicy", 3},
                                                           {"get_mempolicy", 5},
                                                           {"mq_open", 4},
                                                           {"mq_unlink", 1},
                                                           {"mq_timedsend", 5},
                                                           {"mq_timedreceive", 5},
                                                           {"mq_notify", 2},
                                                           {"mq_getsetattr", 3},
                                                           {"kexec_load", 4},
                                                           {"waitid", 5},
                                                           {"add_key", 5},
                                                           {"request_key", 4},
                                                           {"keyctl", 5},
                                                           {"ioprio_set", 3},
                                                           {"ioprio_get", 2},
                                                           {"inotify_init", 0},
                                                           {"inotify_add_watch", 3},
                                                           {"inotify_rm_watch", 2},
                                                           {"migrate_pages", 4},
                                                           {"openat", 4},
                                                           {"mkdirat", 3},
                                                           {"mknodat", 4},
                                                           {"fchownat", 5},
                                                           {"futimesat", 3},
                                                           {"newfstatat", 4},
                                                           {"unlinkat", 3},
                                                           {"renameat", 4},
                                                           {"linkat", 5},
                                                           {"symlinkat", 3},
                                                           {"readlinkat", 4},
                                                           {"fchmodat", 3},
                                                           {"faccessat", 3},
                                                           {"pselect6", 6},
                                                           {"ppoll", 5},
                                                           {"unshare", 1},
                                                           {"set_robust_list", 2},
                                                           {"get_robust_list", 3},
                                                           {"splice", 6},
                                                           {"tee", 4},
                                                           {"sync_file_range", 4},
                                                           {"vmsplice", 4},
                                                           {"move_pages", 6},
                                                           {"utimensat", 4},
                                                           {"epoll_pwait", 6},
                                                           {"signalfd", 3},
                                                           {"timerfd_create", 2},
                                                           {"eventfd", 1},
                                                           {"fallocate", 4},
                                                           {"timerfd_settime", 4},
                                                           {"timerfd_gettime", 2},
                                                           {"accept4", 4},
                                                           {"signalfd4", 4},
                                                           {"eventfd2", 2},
                                                           {"epoll_create1", 1},
                                                           {"dup3", 3},
                                                           {"pipe2", 2},
                                                           {"inotify_init1", 1},
                                                           {"preadv", 5},
                                                           {"pwritev", 5},
                                                           {"rt_tgsigqueueinfo", 4},
                                                           {"perf_event_open", 5},
                                                           {"recvmmsg", 5},
                                                           {"fanotify_init", 2},
                                                           {"fanotify_mark", 5},
                                                           {"prlimit64", 4},
                                                           {"name_to_handle_at", 5},
                                                           {"open_by_handle_at", 3},
                                                           {"clock_adjtime", 2},
                                                           {"syncfs", 1},
                                                           {"sendmmsg", 4},
                                                           {"setns", 2},
                                                           {"getcpu", 3},
                                                           {"process_vm_readv", 6},
                                                           {"process_vm_writev", 6},
                                                           {"kcmp", 5},
                                                           {"finit_module", 3},
                                                           {"sched_setattr", 3},
                                                           {"sched_getattr", 4},
                                                           {"renameat2", 5},
                                                           {"seccomp", 3},
                                                           {"getrandom", 3},
                                                           {"memfd_create", 2},
                                                           {"kexec_file_load", 5},
                                                           {"bpf", 3},
                                                           {"execveat", 5},
                                                           {"userfaultfd", 1},
                                                           {"membarrier", 3},
                                                           {"mlock2", 3},
                                                           {"copy_file_range", 6},
                                                           {"preadv2", 6},
                                                           {"pwritev2", 6},
                                                           {"pkey_mprotect", 4},
                                                           {"pkey_alloc", 2},
                                                           {"pkey_free", 1},
                                                           {"statx", 5},
                                                           {"io_pgetevents", 6},
                                                           {"rseq", 4},
                                                           {"uretprobe", 0},
                                                           {"pidfd_send_signal", 4},
                                                           {"io_uring_setup", 2},
                                                           {"io_uring_enter", 6},
                                                           {"io_uring_register", 4},
                                                           {"open_tree", 3},
                                                           {"move_mount", 5},
                                                           {"fsopen", 2},
                                                           {"fsconfig", 5},
                                                           {"fsmount", 3},
                                                           {"fspick", 3},
                                                           {"pidfd_open", 2},
                                                           {"clone3", 2},
                                                           {"close_range", 3},
                                                           {"openat2", 4},
                                                           {"pidfd_getfd", 3},
                                                           {"faccessat2", 4},
                                                           {"process_madvise", 5},
                                                           {"epoll_pwait2", 6},
                                                           {"mount_setattr", 5},
                                                           {"quotactl_fd", 4},
                                                           {"landlock_create_ruleset", 3},
                                                           {"landlock_add_rule", 4},
                                                           {"landlock_restrict_self", 2},
                                                           {"memfd_secret", 1},
                                                           {"process_mrelease", 2},
                                                           {"futex_waitv", 5},
                                                           {"set_mempolicy_home_node", 4},
                                                           {"cachestat", 4},
                                                           {"fchmodat2", 4},
                                                           {"map_shadow_stack", 3},
                                                           {"futex_wake", 4},
                                                           {"futex_wait", 6},
                                                           {"futex_requeue", 4},
                                                           {"statmount", 4},
                                                           {"listmount", 4},
                                                           {"lsm_get_self_attr", 4},
                                                           {"lsm_set_self_attr", 4},
                                                           {"lsm_list_modules", 3},
                                                           {"mseal", 3}};

std::unique_ptr<LinuxSyscallManager> LinuxSyscallManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::move(std::make_unique<LinuxSyscallManager>(arion));
}

LinuxSyscallManager::LinuxSyscallManager(std::weak_ptr<Arion> arion) : arion(arion)
{
    std::shared_ptr<Arion> arion_ = this->arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");

    this->init_syscall_funcs();
}

void LinuxSyscallManager::add_syscall_entry(std::string name, SYSCALL_FUNC func)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (arion->abi->has_syscall_with_name(name))
        this->syscall_funcs[arion->abi->get_syscall_no_by_name(name)] = func;
}

void LinuxSyscallManager::init_syscall_funcs()
{
    this->add_syscall_entry("read", sys_read);
    this->add_syscall_entry("write", sys_write);
    this->add_syscall_entry("open", sys_open);
    this->add_syscall_entry("close", sys_close);
    this->add_syscall_entry("newstat", sys_newstat);
    this->add_syscall_entry("newfstat", sys_newfstat);
    this->add_syscall_entry("newlstat", sys_newlstat);
    this->add_syscall_entry("poll", sys_poll);
    this->add_syscall_entry("lseek", sys_lseek);
    this->add_syscall_entry("mmap", sys_mmap);
    this->add_syscall_entry("mmap2", sys_mmap2);
    this->add_syscall_entry("mmap_pgoff", sys_mmap2);
    this->add_syscall_entry("mprotect", sys_mprotect);
    this->add_syscall_entry("munmap", sys_munmap);
    this->add_syscall_entry("brk", sys_brk);
    this->add_syscall_entry("rt_sigaction", sys_rt_sigaction);
    this->add_syscall_entry("ioctl", sys_ioctl);
    this->add_syscall_entry("pread64", sys_pread64);
    this->add_syscall_entry("pwrite64", sys_pwrite64);
    this->add_syscall_entry("readv", sys_readv);
    this->add_syscall_entry("writev", sys_writev);
    this->add_syscall_entry("access", sys_access);
    this->add_syscall_entry("pselect6", sys_pselect6);
    this->add_syscall_entry("dup", sys_dup);
    this->add_syscall_entry("dup2", sys_dup2);
    this->add_syscall_entry("pause", sys_pause);
    this->add_syscall_entry("getpid", sys_getpid);
    this->add_syscall_entry("socket", sys_socket);
    this->add_syscall_entry("socketcall", sys_socketcall);
    this->add_syscall_entry("connect", sys_connect);
    this->add_syscall_entry("accept", sys_accept);
    this->add_syscall_entry("sendto", sys_sendto);
    this->add_syscall_entry("send", sys_send);
    this->add_syscall_entry("recvfrom", sys_recvfrom);
    this->add_syscall_entry("recv", sys_recv);
    this->add_syscall_entry("sendmsg", sys_sendmsg);
    this->add_syscall_entry("recvmsg", sys_recvmsg);
    this->add_syscall_entry("shutdown", sys_shutdown);
    this->add_syscall_entry("bind", sys_bind);
    this->add_syscall_entry("listen", sys_listen);
    this->add_syscall_entry("getsockname", sys_getsockname);
    this->add_syscall_entry("getpeername", sys_getpeername);
    this->add_syscall_entry("socketpair", sys_socketpair);
    this->add_syscall_entry("setsockopt", sys_setsockopt);
    this->add_syscall_entry("getsockopt", sys_getsockopt);
    this->add_syscall_entry("clone", sys_clone);
    this->add_syscall_entry("fork", sys_fork);
    this->add_syscall_entry("execve", sys_execve);
    this->add_syscall_entry("exit", sys_exit);
    this->add_syscall_entry("wait4", sys_wait4);
    this->add_syscall_entry("kill", sys_kill);
    this->add_syscall_entry("newuname", sys_newuname);
    this->add_syscall_entry("fcntl", sys_fcntl);
    this->add_syscall_entry("truncate", sys_truncate);
    this->add_syscall_entry("ftruncate", sys_ftruncate);
    this->add_syscall_entry("faccessat", sys_faccessat);
    this->add_syscall_entry("getcwd", sys_getcwd);
    this->add_syscall_entry("chdir", sys_chdir);
    this->add_syscall_entry("fchdir", sys_fchdir);
    this->add_syscall_entry("rename", sys_rename);
    this->add_syscall_entry("mkdir", sys_mkdir);
    this->add_syscall_entry("rmdir", sys_rmdir);
    this->add_syscall_entry("creat", sys_creat);
    this->add_syscall_entry("link", sys_link);
    this->add_syscall_entry("unlink", sys_unlink);
    this->add_syscall_entry("symlink", sys_symlink);
    this->add_syscall_entry("readlink", sys_readlink);
    this->add_syscall_entry("gettimeofday", sys_gettimeofday);
    this->add_syscall_entry("getrlimit", sys_getrlimit);
    this->add_syscall_entry("old_getrlimit", sys_getrlimit);
    this->add_syscall_entry("sysinfo", sys_sysinfo);
    this->add_syscall_entry("getuid", sys_getuid);
    this->add_syscall_entry("getgid", sys_getgid);
    this->add_syscall_entry("setuid", sys_setuid);
    this->add_syscall_entry("setgid", sys_setgid);
    this->add_syscall_entry("geteuid", sys_geteuid);
    this->add_syscall_entry("getegid", sys_getegid);
    this->add_syscall_entry("setpgid", sys_setpgid);
    this->add_syscall_entry("getppid", sys_getppid);
    this->add_syscall_entry("getpgrp", sys_getpgrp);
    this->add_syscall_entry("setsid", sys_setsid);
    this->add_syscall_entry("setreuid", sys_setreuid);
    this->add_syscall_entry("setregid", sys_setregid);
    this->add_syscall_entry("getgroups", sys_getgroups);
    this->add_syscall_entry("setgroups", sys_setgroups);
    this->add_syscall_entry("setresuid", sys_setresuid);
    this->add_syscall_entry("getresuid", sys_getresuid);
    this->add_syscall_entry("setresgid", sys_setresgid);
    this->add_syscall_entry("getresgid", sys_getresgid);
    this->add_syscall_entry("getpgid", sys_getpgid);
    this->add_syscall_entry("setfsuid", sys_setfsuid);
    this->add_syscall_entry("setfsgid", sys_setfsgid);
    this->add_syscall_entry("getsid", sys_getsid);
    this->add_syscall_entry("capget", sys_capget);
    this->add_syscall_entry("capset", sys_capset);
    this->add_syscall_entry("statfs", sys_statfs);
    this->add_syscall_entry("fstatfs", sys_fstatfs);
    this->add_syscall_entry("sched_rr_get_interval", sys_sched_rr_get_interval);
    this->add_syscall_entry("arch_prctl", sys_arch_prctl);
    this->add_syscall_entry("gettid", sys_gettid);
    this->add_syscall_entry("time", sys_time);
    this->add_syscall_entry("futex", sys_futex);
    this->add_syscall_entry("getdents64", sys_getdents64);
    this->add_syscall_entry("set_tid_address", sys_set_tid_address);
    this->add_syscall_entry("set_thread_area", sys_set_thread_area);
    this->add_syscall_entry("set_tls", sys_set_tls);
    this->add_syscall_entry("clock_gettime", sys_clock_gettime);
    this->add_syscall_entry("clock_getres", sys_clock_getres);
    this->add_syscall_entry("clock_nanosleep", sys_clock_nanosleep);
    this->add_syscall_entry("exit_group", sys_exit_group);
    this->add_syscall_entry("tgkill", sys_tgkill);
    this->add_syscall_entry("waitid", sys_waitid);
    this->add_syscall_entry("openat", sys_openat);
    this->add_syscall_entry("newfstatat", sys_newfstatat);
    this->add_syscall_entry("renameat", sys_renameat);
    this->add_syscall_entry("readlinkat", sys_readlinkat);
    this->add_syscall_entry("pselect6", sys_pselect6);
    this->add_syscall_entry("ppoll", sys_ppoll);
    this->add_syscall_entry("accept4", sys_accept4);
    this->add_syscall_entry("recvmmsg", sys_recvmmsg);
    this->add_syscall_entry("prlimit64", sys_prlimit64);
    this->add_syscall_entry("sendmmsg", sys_sendmmsg);
    this->add_syscall_entry("getcpu", sys_getcpu);
    this->add_syscall_entry("renameat2", sys_renameat2);
    this->add_syscall_entry("getrandom", sys_getrandom);
    this->add_syscall_entry("statx", sys_statx);
    this->add_syscall_entry("clone3", sys_clone3);
}

void LinuxSyscallManager::process_syscall(std::shared_ptr<Arion> arion)
{
    REG sysno_reg = arion->abi->get_attrs()->syscalling_conv.sysno_reg;
    REG syscall_ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    uint64_t sysno = arion->abi->read_arch_reg(sysno_reg);
    SYSCALL_FUNC func = arion->syscalls->get_syscall_func(sysno);
    if (func == nullptr)
    {
        arion->logger->warn(std::string("No associated syscall for sysno ") + int_to_hex<uint64_t>(sysno) +
                            std::string("."));
        arion->abi->write_arch_reg(syscall_ret_reg, 0);
        return;
    }
    std::string syscall_name = arion->abi->get_name_by_syscall_no(sysno);
    arion->logger->debug(std::string("SYSCALL : ") + syscall_name);
    uint8_t params_n = PARAMS_N_BY_SYSCALL_NAME.at(syscall_name);
    std::vector<SYS_PARAM> func_params;
    std::vector<REG> sys_regs = arion->abi->get_attrs()->syscalling_conv.sys_param_regs;
    for (uint8_t param_i = 0; param_i < params_n; param_i++)
    {
        REG param_reg = sys_regs.at(param_i);
        uint64_t param_val = arion->abi->read_arch_reg(param_reg);
        func_params.push_back(param_val);
    }
    uint64_t syscall_ret = func(arion, func_params);
    arion->abi->write_arch_reg(syscall_ret_reg, syscall_ret);
}

void LinuxSyscallManager::set_syscall_func(uint64_t sysno, SYSCALL_FUNC func)
{
    this->syscall_funcs[sysno] = func;
}

void LinuxSyscallManager::set_syscall_func(std::string name, SYSCALL_FUNC func)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint64_t sysno = arion->abi->get_syscall_no_by_name(name);
    this->set_syscall_func(sysno, func);
}

SYSCALL_FUNC LinuxSyscallManager::get_syscall_func(uint64_t sysno)
{
    auto it = this->syscall_funcs.find(sysno);
    if (it != this->syscall_funcs.end())
        return it->second;
    return nullptr;
}

SYSCALL_FUNC LinuxSyscallManager::get_syscall_func(std::string name)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint64_t sysno = arion->abi->get_syscall_no_by_name(name);
    return this->get_syscall_func(sysno);
}
