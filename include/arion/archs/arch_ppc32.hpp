#ifndef ARION_ARCH_PPC32_HPP
#define ARION_ARCH_PPC32_HPP

#include <arion/common/arch_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <cstdint>
#include <map>
#include <string>

namespace arion_ppc32
{

/// A map identifying a PowerPC32 syscall name given its number.
inline std::map<uint64_t, std::string> NAME_BY_SYSCALL_NO = {
    {0, "restart_syscall"}, {1, "exit"}, {2, "fork"}, {3, "read"}, {4, "write"}, {5, "open"}, {6, "close"},
    {7, "waitpid"}, {8, "creat"}, {9, "link"}, {10, "unlink"}, {11, "execve"}, {12, "chdir"}, {13, "time"},
    {14, "mknod"}, {15, "chmod"}, {16, "lchown"}, {17, "break"}, {18, "oldstat"}, {19, "lseek"}, {20, "getpid"},
    {21, "mount"}, {22, "umount"}, {23, "setuid"}, {24, "getuid"}, {25, "stime"}, {26, "ptrace"}, {27, "alarm"},
    {28, "oldfstat"}, {29, "pause"}, {30, "utime"}, {31, "stty"}, {32, "gtty"}, {33, "access"}, {34, "nice"},
    {35, "ftime"}, {36, "sync"}, {37, "kill"}, {38, "rename"}, {39, "mkdir"}, {40, "rmdir"}, {41, "dup"}, {42, "pipe"},
    {43, "times"}, {44, "prof"}, {45, "brk"}, {46, "setgid"}, {47, "getgid"}, {48, "signal"}, {49, "geteuid"},
    {50, "getegid"}, {51, "acct"}, {52, "umount2"}, {53, "lock"}, {54, "ioctl"}, {55, "fcntl"}, {56, "mpx"},
    {57, "setpgid"}, {58, "ulimit"}, {59, "oldolduname"}, {60, "umask"}, {61, "chroot"}, {62, "ustat"}, {63, "dup2"},
    {64, "getppid"}, {65, "getpgrp"}, {66, "setsid"}, {67, "sigaction"}, {68, "sgetmask"}, {69, "ssetmask"},
    {70, "setreuid"}, {71, "setregid"}, {72, "sigsuspend"}, {73, "sigpending"}, {74, "sethostname"}, {75, "setrlimit"},
    {76, "getrlimit"}, {77, "getrusage"}, {78, "gettimeofday"}, {79, "settimeofday"}, {80, "getgroups"},
    {81, "setgroups"}, {82, "select"}, {83, "symlink"}, {84, "oldlstat"}, {85, "readlink"}, {86, "uselib"},
    {87, "swapon"}, {88, "reboot"}, {89, "readdir"}, {90, "mmap"}, {91, "munmap"}, {92, "truncate"}, {93, "ftruncate"},
    {94, "fchmod"}, {95, "fchown"}, {96, "getpriority"}, {97, "setpriority"}, {98, "profil"}, {99, "statfs"},
    {100, "fstatfs"}, {101, "ioperm"}, {102, "socketcall"}, {103, "syslog"}, {104, "setitimer"}, {105, "getitimer"},
    {106, "stat"}, {107, "lstat"}, {108, "fstat"}, {109, "olduname"}, {110, "iopl"}, {111, "vhangup"}, {112, "idle"},
    {113, "vm86"}, {114, "wait4"}, {115, "swapoff"}, {116, "sysinfo"}, {117, "ipc"}, {118, "fsync"}, {119, "sigreturn"},
    {120, "clone"}, {121, "setdomainname"}, {122, "uname"}, {123, "modify_ldt"}, {124, "adjtimex"}, {125, "mprotect"},
    {126, "sigprocmask"}, {127, "create_module"}, {128, "init_module"}, {129, "delete_module"},
    {130, "get_kernel_syms"}, {131, "quotactl"}, {132, "getpgid"}, {133, "fchdir"}, {134, "bdflush"}, {135, "sysfs"},
    {136, "personality"}, {137, "afs_syscall"}, {138, "setfsuid"}, {139, "setfsgid"}, {140, "_llseek"},
    {141, "getdents"}, {142, "_newselect"}, {143, "flock"}, {144, "msync"}, {145, "readv"}, {146, "writev"},
    {147, "getsid"}, {148, "fdatasync"}, {149, "_sysctl"}, {150, "mlock"}, {151, "munlock"}, {152, "mlockall"},
    {153, "munlockall"}, {154, "sched_setparam"}, {155, "sched_getparam"}, {156, "sched_setscheduler"},
    {157, "sched_getscheduler"}, {158, "sched_yield"}, {159, "sched_get_priority_max"}, {160, "sched_get_priority_min"},
    {161, "sched_rr_get_interval"}, {162, "nanosleep"}, {163, "mremap"}, {164, "setresuid"}, {165, "getresuid"},
    {166, "query_module"}, {167, "poll"}, {168, "nfsservctl"}, {169, "setresgid"}, {170, "getresgid"}, {171, "prctl"},
    {172, "rt_sigreturn"}, {173, "rt_sigaction"}, {174, "rt_sigprocmask"}, {175, "rt_sigpending"},
    {176, "rt_sigtimedwait"}, {177, "rt_sigqueueinfo"}, {178, "rt_sigsuspend"}, {179, "pread64"}, {180, "pwrite64"},
    {181, "chown"}, {182, "getcwd"}, {183, "capget"}, {184, "capset"}, {185, "sigaltstack"}, {186, "sendfile"},
    {187, "getpmsg"}, {188, "putpmsg"}, {189, "vfork"}, {190, "ugetrlimit"}, {191, "readahead"}, {192, "mmap2"},
    {193, "truncate64"}, {194, "ftruncate64"}, {195, "stat64"}, {196, "lstat64"}, {197, "fstat64"},
    {198, "pciconfig_read"}, {199, "pciconfig_write"}, {200, "pciconfig_iobase"}, {201, "multiplexer"},
    {202, "getdents64"}, {203, "pivot_root"}, {204, "fcntl64"}, {205, "madvise"}, {206, "mincore"}, {207, "gettid"},
    {208, "tkill"}, {209, "setxattr"}, {210, "lsetxattr"}, {211, "fsetxattr"}, {212, "getxattr"}, {213, "lgetxattr"},
    {214, "fgetxattr"}, {215, "listxattr"}, {216, "llistxattr"}, {217, "flistxattr"}, {218, "removexattr"},
    {219, "lremovexattr"}, {220, "fremovexattr"}, {221, "futex"}, {222, "sched_setaffinity"},
    {223, "sched_getaffinity"}, {225, "tuxcall"}, {226, "sendfile64"}, {227, "io_setup"}, {228, "io_destroy"},
    {229, "io_getevents"}, {230, "io_submit"}, {231, "io_cancel"}, {232, "set_tid_address"}, {233, "fadvise64"},
    {234, "exit_group"}, {235, "lookup_dcookie"}, {236, "epoll_create"}, {237, "epoll_ctl"}, {238, "epoll_wait"},
    {239, "remap_file_pages"}, {240, "timer_create"}, {241, "timer_settime"}, {242, "timer_gettime"},
    {243, "timer_getoverrun"}, {244, "timer_delete"}, {245, "clock_settime"}, {246, "clock_gettime"},
    {247, "clock_getres"}, {248, "clock_nanosleep"}, {249, "swapcontext"}, {250, "tgkill"}, {251, "utimes"},
    {252, "statfs64"}, {253, "fstatfs64"}, {254, "fadvise64_64"}, {255, "rtas"}, {256, "sys_debug_setcontext"},
    {258, "migrate_pages"}, {259, "mbind"}, {260, "get_mempolicy"}, {261, "set_mempolicy"}, {262, "mq_open"},
    {263, "mq_unlink"}, {264, "mq_timedsend"}, {265, "mq_timedreceive"}, {266, "mq_notify"}, {267, "mq_getsetattr"},
    {268, "kexec_load"}, {269, "add_key"}, {270, "request_key"}, {271, "keyctl"}, {272, "waitid"}, {273, "ioprio_set"},
    {274, "ioprio_get"}, {275, "inotify_init"}, {276, "inotify_add_watch"}, {277, "inotify_rm_watch"}, {278, "spu_run"},
    {279, "spu_create"}, {280, "pselect6"}, {281, "ppoll"}, {282, "unshare"}, {283, "splice"}, {284, "tee"},
    {285, "vmsplice"}, {286, "openat"}, {287, "mkdirat"}, {288, "mknodat"}, {289, "fchownat"}, {290, "futimesat"},
    {291, "fstatat64"}, {292, "unlinkat"}, {293, "renameat"}, {294, "linkat"}, {295, "symlinkat"}, {296, "readlinkat"},
    {297, "fchmodat"}, {298, "faccessat"}, {299, "get_robust_list"}, {300, "set_robust_list"}, {301, "move_pages"},
    {302, "getcpu"}, {303, "epoll_pwait"}, {304, "utimensat"}, {305, "signalfd"}, {306, "timerfd_create"},
    {307, "eventfd"}, {308, "sync_file_range2"}, {309, "fallocate"}, {310, "subpage_prot"}, {311, "timerfd_settime"},
    {312, "timerfd_gettime"}, {313, "signalfd4"}, {314, "eventfd2"}, {315, "epoll_create1"}, {316, "dup3"},
    {317, "pipe2"}, {318, "inotify_init1"}, {319, "perf_event_open"}, {320, "preadv"}, {321, "pwritev"},
    {322, "rt_tgsigqueueinfo"}, {323, "fanotify_init"}, {324, "fanotify_mark"}, {325, "prlimit64"}, {326, "socket"},
    {327, "bind"}, {328, "connect"}, {329, "listen"}, {330, "accept"}, {331, "getsockname"}, {332, "getpeername"},
    {333, "socketpair"}, {334, "send"}, {335, "sendto"}, {336, "recv"}, {337, "recvfrom"}, {338, "shutdown"},
    {339, "setsockopt"}, {340, "getsockopt"}, {341, "sendmsg"}, {342, "recvmsg"}, {343, "recvmmsg"}, {344, "accept4"},
    {345, "name_to_handle_at"}, {346, "open_by_handle_at"}, {347, "clock_adjtime"}, {348, "syncfs"}, {349, "sendmmsg"},
    {350, "setns"}, {351, "process_vm_readv"}, {352, "process_vm_writev"}, {353, "finit_module"}, {354, "kcmp"},
    {355, "sched_setattr"}, {356, "sched_getattr"}, {357, "renameat2"}, {358, "seccomp"}, {359, "getrandom"},
    {360, "memfd_create"}, {361, "bpf"}, {362, "execveat"}, {363, "switch_endian"}, {364, "userfaultfd"},
    {365, "membarrier"}, {378, "mlock2"}, {379, "copy_file_range"}, {380, "preadv2"}, {381, "pwritev2"},
    {382, "kexec_file_load"}, {383, "statx"}, {384, "pkey_alloc"}, {385, "pkey_free"}, {386, "pkey_mprotect"},
    {387, "rseq"}, {388, "io_pgetevents"}, {392, "semtimedop"}, {393, "semget"}, {394, "semctl"}, {395, "shmget"},
    {396, "shmctl"}, {397, "shmat"}, {398, "shmdt"}, {399, "msgget"}, {400, "msgsnd"}, {401, "msgrcv"}, {402, "msgctl"},
    {403, "clock_gettime64"}, {404, "clock_settime64"}, {405, "clock_adjtime64"}, {406, "clock_getres_time64"},
    {407, "clock_nanosleep_time64"}, {408, "timer_gettime64"}, {409, "timer_settime64"}, {410, "timerfd_gettime64"},
    {411, "timerfd_settime64"}, {412, "utimensat_time64"}, {413, "pselect6_time64"}, {414, "ppoll_time64"},
    {416, "io_pgetevents_time64"}, {417, "recvmmsg_time64"}, {418, "mq_timedsend_time64"},
    {419, "mq_timedreceive_time64"}, {420, "semtimedop_time64"}, {421, "rt_sigtimedwait_time64"}, {422, "futex_time64"},
    {423, "sched_rr_get_interval_time64"}, {424, "pidfd_send_signal"}, {425, "io_uring_setup"}, {426, "io_uring_enter"},
    {427, "io_uring_register"}, {428, "open_tree"}, {429, "move_mount"}, {430, "fsopen"}, {431, "fsconfig"},
    {432, "fsmount"}, {433, "fspick"}, {434, "pidfd_open"}, {435, "clone3"}, {436, "close_range"}, {437, "openat2"},
    {438, "pidfd_getfd"}, {439, "faccessat2"}, {440, "process_madvise"}, {441, "epoll_pwait2"}, {442, "mount_setattr"},
    {443, "quotactl_fd"}, {444, "landlock_create_ruleset"}, {445, "landlock_add_rule"}, {446, "landlock_restrict_self"},
    {448, "process_mrelease"}, {449, "futex_waitv"}, {450, "set_mempolicy_home_node"}, {451, "cachestat"},
    {452, "fchmodat2"}, {453, "map_shadow_stack"}, {454, "futex_wake"}, {455, "futex_wait"}, {456, "futex_requeue"},
    {457, "statmount"}, {458, "listmount"}, {459, "lsm_get_self_attr"}, {460, "lsm_set_self_attr"},
    {461, "lsm_list_modules"}, {462, "mseal"}, {463, "setxattrat"}, {464, "getxattrat"}, {465, "listxattrat"},
    {466, "removexattrat"}, {467, "open_tree_attr"}, {468, "file_getattr"}, {469, "file_setattr"}};

/// A map identifying a Unicorn PowerPC register given its name.
inline std::map<std::string, arion::REG> ARCH_REGS = {
    {"R0", UC_PPC_REG_0},     {"R1", UC_PPC_REG_1},     {"R2", UC_PPC_REG_2},     {"R3", UC_PPC_REG_3},
    {"R4", UC_PPC_REG_4},     {"R5", UC_PPC_REG_5},     {"R6", UC_PPC_REG_6},     {"R7", UC_PPC_REG_7},
    {"R8", UC_PPC_REG_8},     {"R9", UC_PPC_REG_9},     {"R10", UC_PPC_REG_10},   {"R11", UC_PPC_REG_11},
    {"R12", UC_PPC_REG_12},   {"R13", UC_PPC_REG_13},   {"R14", UC_PPC_REG_14},   {"R15", UC_PPC_REG_15},
    {"R16", UC_PPC_REG_16},   {"R17", UC_PPC_REG_17},   {"R18", UC_PPC_REG_18},   {"R19", UC_PPC_REG_19},
    {"R20", UC_PPC_REG_20},   {"R21", UC_PPC_REG_21},   {"R22", UC_PPC_REG_22},   {"R23", UC_PPC_REG_23},
    {"R24", UC_PPC_REG_24},   {"R25", UC_PPC_REG_25},   {"R26", UC_PPC_REG_26},   {"R27", UC_PPC_REG_27},
    {"R28", UC_PPC_REG_28},   {"R29", UC_PPC_REG_29},   {"R30", UC_PPC_REG_30},   {"R31", UC_PPC_REG_31},
    {"PC", UC_PPC_REG_PC},    {"SP", UC_PPC_REG_1},     {"LR", UC_PPC_REG_LR},    {"CTR", UC_PPC_REG_CTR},
    {"XER", UC_PPC_REG_XER},  {"MSR", UC_PPC_REG_MSR},  {"CR", UC_PPC_REG_CR},    {"FPR0", UC_PPC_REG_FPR0},
    {"FPR1", UC_PPC_REG_FPR1}, {"FPR2", UC_PPC_REG_FPR2}, {"FPR3", UC_PPC_REG_FPR3}, {"FPR4", UC_PPC_REG_FPR4},
    {"FPR5", UC_PPC_REG_FPR5}, {"FPR6", UC_PPC_REG_FPR6}, {"FPR7", UC_PPC_REG_FPR7}, {"FPR8", UC_PPC_REG_FPR8},
    {"FPR9", UC_PPC_REG_FPR9}, {"FPR10", UC_PPC_REG_FPR10}, {"FPR11", UC_PPC_REG_FPR11}, {"FPR12", UC_PPC_REG_FPR12},
    {"FPR13", UC_PPC_REG_FPR13}, {"FPR14", UC_PPC_REG_FPR14}, {"FPR15", UC_PPC_REG_FPR15}, {"FPR16", UC_PPC_REG_FPR16},
    {"FPR17", UC_PPC_REG_FPR17}, {"FPR18", UC_PPC_REG_FPR18}, {"FPR19", UC_PPC_REG_FPR19}, {"FPR20", UC_PPC_REG_FPR20},
    {"FPR21", UC_PPC_REG_FPR21}, {"FPR22", UC_PPC_REG_FPR22}, {"FPR23", UC_PPC_REG_FPR23}, {"FPR24", UC_PPC_REG_FPR24},
    {"FPR25", UC_PPC_REG_FPR25}, {"FPR26", UC_PPC_REG_FPR26}, {"FPR27", UC_PPC_REG_FPR27}, {"FPR28", UC_PPC_REG_FPR28},
    {"FPR29", UC_PPC_REG_FPR29}, {"FPR30", UC_PPC_REG_FPR30}, {"FPR31", UC_PPC_REG_FPR31}, {"FPSCR", UC_PPC_REG_FPSCR}};

/// A map identifying a Unicorn PowerPC register size given the register.
inline std::map<arion::REG, uint8_t> ARCH_REGS_SZ = {
    {UC_PPC_REG_0, 4},    {UC_PPC_REG_1, 4},    {UC_PPC_REG_2, 4},    {UC_PPC_REG_3, 4},
    {UC_PPC_REG_4, 4},    {UC_PPC_REG_5, 4},    {UC_PPC_REG_6, 4},    {UC_PPC_REG_7, 4},
    {UC_PPC_REG_8, 4},    {UC_PPC_REG_9, 4},    {UC_PPC_REG_10, 4},   {UC_PPC_REG_11, 4},
    {UC_PPC_REG_12, 4},   {UC_PPC_REG_13, 4},   {UC_PPC_REG_14, 4},   {UC_PPC_REG_15, 4},
    {UC_PPC_REG_16, 4},   {UC_PPC_REG_17, 4},   {UC_PPC_REG_18, 4},   {UC_PPC_REG_19, 4},
    {UC_PPC_REG_20, 4},   {UC_PPC_REG_21, 4},   {UC_PPC_REG_22, 4},   {UC_PPC_REG_23, 4},
    {UC_PPC_REG_24, 4},   {UC_PPC_REG_25, 4},   {UC_PPC_REG_26, 4},   {UC_PPC_REG_27, 4},
    {UC_PPC_REG_28, 4},   {UC_PPC_REG_29, 4},   {UC_PPC_REG_30, 4},   {UC_PPC_REG_31, 4},
    {UC_PPC_REG_PC, 4},   {UC_PPC_REG_LR, 4},   {UC_PPC_REG_CTR, 4},  {UC_PPC_REG_XER, 4},
    {UC_PPC_REG_MSR, 4},  {UC_PPC_REG_CR, 4},   {UC_PPC_REG_FPR0, 8}, {UC_PPC_REG_FPR1, 8},
    {UC_PPC_REG_FPR2, 8}, {UC_PPC_REG_FPR3, 8}, {UC_PPC_REG_FPR4, 8}, {UC_PPC_REG_FPR5, 8},
    {UC_PPC_REG_FPR6, 8}, {UC_PPC_REG_FPR7, 8}, {UC_PPC_REG_FPR8, 8}, {UC_PPC_REG_FPR9, 8},
    {UC_PPC_REG_FPR10, 8}, {UC_PPC_REG_FPR11, 8}, {UC_PPC_REG_FPR12, 8}, {UC_PPC_REG_FPR13, 8},
    {UC_PPC_REG_FPR14, 8}, {UC_PPC_REG_FPR15, 8}, {UC_PPC_REG_FPR16, 8}, {UC_PPC_REG_FPR17, 8},
    {UC_PPC_REG_FPR18, 8}, {UC_PPC_REG_FPR19, 8}, {UC_PPC_REG_FPR20, 8}, {UC_PPC_REG_FPR21, 8},
    {UC_PPC_REG_FPR22, 8}, {UC_PPC_REG_FPR23, 8}, {UC_PPC_REG_FPR24, 8}, {UC_PPC_REG_FPR25, 8},
    {UC_PPC_REG_FPR26, 8}, {UC_PPC_REG_FPR27, 8}, {UC_PPC_REG_FPR28, 8}, {UC_PPC_REG_FPR29, 8},
    {UC_PPC_REG_FPR30, 8}, {UC_PPC_REG_FPR31, 8}, {UC_PPC_REG_FPSCR, 8}};

/// The list of high-level PowerPC32 registers that make up the context to be saved and restored.
inline std::vector<arion::REG> CTXT_REGS = {
    UC_PPC_REG_0,  UC_PPC_REG_1,  UC_PPC_REG_2,  UC_PPC_REG_3,  UC_PPC_REG_4,  UC_PPC_REG_5,  UC_PPC_REG_6,
    UC_PPC_REG_7,  UC_PPC_REG_8,  UC_PPC_REG_9,  UC_PPC_REG_10, UC_PPC_REG_11, UC_PPC_REG_12, UC_PPC_REG_13,
    UC_PPC_REG_14, UC_PPC_REG_15, UC_PPC_REG_16, UC_PPC_REG_17, UC_PPC_REG_18, UC_PPC_REG_19, UC_PPC_REG_20,
    UC_PPC_REG_21, UC_PPC_REG_22, UC_PPC_REG_23, UC_PPC_REG_24, UC_PPC_REG_25, UC_PPC_REG_26, UC_PPC_REG_27,
    UC_PPC_REG_28, UC_PPC_REG_29, UC_PPC_REG_30, UC_PPC_REG_31, UC_PPC_REG_PC, UC_PPC_REG_LR, UC_PPC_REG_CTR,
    UC_PPC_REG_XER, UC_PPC_REG_CR, UC_PPC_REG_MSR, UC_PPC_REG_FPR0,  UC_PPC_REG_FPR1,  UC_PPC_REG_FPR2,
    UC_PPC_REG_FPR3, UC_PPC_REG_FPR4, UC_PPC_REG_FPR5, UC_PPC_REG_FPR6, UC_PPC_REG_FPR7, UC_PPC_REG_FPR8,
    UC_PPC_REG_FPR9, UC_PPC_REG_FPR10, UC_PPC_REG_FPR11, UC_PPC_REG_FPR12, UC_PPC_REG_FPR13, UC_PPC_REG_FPR14,
    UC_PPC_REG_FPR15, UC_PPC_REG_FPR16, UC_PPC_REG_FPR17, UC_PPC_REG_FPR18, UC_PPC_REG_FPR19, UC_PPC_REG_FPR20,
    UC_PPC_REG_FPR21, UC_PPC_REG_FPR22, UC_PPC_REG_FPR23, UC_PPC_REG_FPR24, UC_PPC_REG_FPR25, UC_PPC_REG_FPR26,
    UC_PPC_REG_FPR27, UC_PPC_REG_FPR28, UC_PPC_REG_FPR29, UC_PPC_REG_FPR30, UC_PPC_REG_FPR31, UC_PPC_REG_FPSCR};

/// PowerPC32 Interrupt Descriptor Table (currently unused).
inline std::map<uint64_t, arion::CPU_INTR> IDT = {};

/// Unicorn PowerPC32 PC and SP registers for genericity.
inline arion::ABI_REGISTERS ABI_REGS = arion::ABI_REGISTERS(UC_PPC_REG_PC, UC_PPC_REG_1);

/// Unicorn PowerPC32 registers involved in calling convention.
inline arion::ABI_CALLING_CONVENTION ABI_CALLING_CONV =
    arion::ABI_CALLING_CONVENTION(UC_PPC_REG_3,
                                  {UC_PPC_REG_3, UC_PPC_REG_4, UC_PPC_REG_5, UC_PPC_REG_6, UC_PPC_REG_7, UC_PPC_REG_8,
                                   UC_PPC_REG_9, UC_PPC_REG_10});

/// Unicorn PowerPC32 registers involved in syscalling convention.
inline arion::ABI_SYSCALLING_CONVENTION ABI_SYSCALLING_CONV = arion::ABI_SYSCALLING_CONVENTION(
    UC_PPC_REG_0, UC_PPC_REG_3,
    {UC_PPC_REG_3, UC_PPC_REG_4, UC_PPC_REG_5, UC_PPC_REG_6, UC_PPC_REG_7, UC_PPC_REG_8, UC_PPC_REG_9, UC_PPC_REG_10});

/// PowerPC32 flags telling the loader which kernel segments should be mapped in memory.
inline arion::KERNEL_SEG_FLAGS SEG_FLAGS = ARION_VVAR_PRESENT | ARION_VDSO_PRESENT;

/// Some well working PowerPC32 chip HWCAP value (set to 0 by default).
inline uint32_t HWCAP = 0;

/// Some well working PowerPC32 chip HWCAP2 value (set to 0 by default).
inline uint32_t HWCAP2 = 0;

/// Size in bytes of a pointer in a PowerPC32 chip.
inline const size_t PTR_SZ = 4;

/// Size in bits of the PowerPC32 general-purpose registers.
inline const uint16_t ARCH_SZ = 32;

/// Arion CPU_ARCH for PowerPC32.
inline arion::CPU_ARCH ARCH = arion::CPU_ARCH::PPC32_ARCH;

/// Multiple architecture specific attributes for PowerPC32, grouped in a structure for genericity purpose.
inline arion::ARCH_ATTRIBUTES ARCH_ATTRS =
    arion::ARCH_ATTRIBUTES(ARCH, ARCH_SZ, PTR_SZ, HWCAP, HWCAP2, SEG_FLAGS, ABI_REGS, ABI_CALLING_CONV,
                           ABI_SYSCALLING_CONV, NAME_BY_SYSCALL_NO);

/// Opcode for the PowerPC32 system call instruction (sc).
inline constexpr uint32_t SC_OPCODE = 0x44000002;

}; // namespace arion_ppc32

namespace arion
{

class Arion;

} // namespace arion

namespace arion_ppc32
{

/// A class responsible for performing architecture specific operations in case of a PowerPC32 chip emulation.
class ArchManagerPPC32 : public arion::ArchManager
{
  private:
    /**
     * Code hook used to detect syscalls.
     * @param[in] arion The Arion instance responsible for the hook.
     * @param[in] address The address of the instruction.
     * @param[in] size The size of the instruction.
     * @param[in] user_data Additional user data.
     */
    static void code_hook(std::shared_ptr<arion::Arion> arion, arion::ADDR address, uint32_t size, void *user_data);

    /**
     * Prepares the ArchManagerPPC32 for emulation.
     */
    void setup() override;

  public:
    /**
     * Builder for ArchManagerPPC32 instances.
     */
    ArchManagerPPC32()
        : ArchManager(std::make_shared<arion::ARCH_ATTRIBUTES>(arion_ppc32::ARCH_ATTRS), arion_ppc32::ARCH_REGS,
                      arion_ppc32::ARCH_REGS_SZ, arion_ppc32::CTXT_REGS, arion_ppc32::IDT, false) {};

    /**
     * Retrieves the Keystone engine associated with this instance, if any.
     * @return The Keystone engine or nullptr.
     */
    ks_engine *curr_ks() override;

    /**
     * Retrieves the Capstone engine associated with this instance.
     * @return The Capstone engine or nullptr.
     */
    csh *curr_cs() override;

    /**
     * Retrieves the current Thread Local Storage (TLS) address from the emulation context.
     * @return The TLS address.
     */
    arion::ADDR dump_tls() override;

    /**
     * Defines a Thread Local Storage (TLS) address to apply to the emulation.
     * @param[in] new_tls The new TLS address.
     */
    void load_tls(arion::ADDR new_tls) override;
};

}; // namespace arion_ppc32

#endif // ARION_ARCH_PPC32_HPP