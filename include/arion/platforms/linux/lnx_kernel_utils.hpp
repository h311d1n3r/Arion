#ifndef ARION_LNX_KERNEL_UTILS_HPP
#define ARION_LNX_KERNEL_UTILS_HPP

#include <arion/utils/type_utils.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/utils/struct_utils.hpp>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/futex.h>
#include <linux/sched.h>
#include <csignal>

// Defined in https://elixir.bootlin.com/linux/v6.12.6/source/arch/x86/include/uapi/asm/signal.h#L93
struct ksigaction
{
    void *handler;
    unsigned long flags;
    void *restorer;
    sigset_t mask;
};

struct rlimit32
{
    uint32_t rlim_cur;
    uint32_t rlim_max;
};

struct iovec32
{
    uint32_t iov_base;
    uint32_t iov_len;
};

arion::PROT_FLAGS kernel_prot_to_arion_prot(int kflags);

class ArionErrCodeType : public ArionType {
public:
    ArionErrCodeType() : ArionType("Error code", ARION_LOG_COLOR::MAGENTA) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionErrCodeType> ARION_ERR_CODE_TYPE;

class ArionFileDescriptorType : public ArionType {
public:
    ArionFileDescriptorType() : ArionType("File descriptor", ARION_LOG_COLOR::MAGENTA) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionFileDescriptorType> ARION_FILE_DESCRIPTOR_TYPE;

class ArionAccessModeType : public ArionFlagType
{
  public:
    ArionAccessModeType() : ArionFlagType("Access mode", {{F_OK, "F_OK"}, {R_OK, "R_OK"}, {W_OK, "W_OK"}, {X_OK, "X_OK"}}) {};
};
extern std::shared_ptr<ArionAccessModeType> ARION_ACCESS_MODE_TYPE;

class ArionOpenModeType : public ArionFlagType
{
  public:
    ArionOpenModeType() : ArionFlagType("Open mode", {{O_RDONLY, "O_RDONLY"}, {O_WRONLY, "O_WRONLY"}, {0x4, "O_EXEC"}, {O_RDWR, "O_RDWR"},
                                                        {O_ACCMODE, "O_ACCMODE"}, {O_LARGEFILE, "O_LARGEFILE"}, {O_CREAT, "O_CREAT"}, {O_APPEND, "O_APPEND"},
                                                        {O_CLOEXEC, "O_CLOEXEC"}, {O_DIRECTORY, "O_DIRECTORY"}, {O_DSYNC, "O_DSYNC"}, {O_EXCL, "O_EXCL"},
                                                        {O_NOCTTY, "O_NOCTTY"}, {O_NOFOLLOW, "O_NOFOLLOW"}, {O_NONBLOCK, "O_NONBLOCK"}, {O_RSYNC, "O_RSYNC"},
                                                        {O_SYNC, "O_SYNC"}, {O_TRUNC, "O_TRUNC"}}) {};
};
extern std::shared_ptr<ArionOpenModeType> ARION_OPEN_MODE_TYPE;

class ArionFileATFlagType : public ArionFlagType
{
  public:
    ArionFileATFlagType() : ArionFlagType("File AT flag", {{AT_SYMLINK_NOFOLLOW, "AT_SYMLINK_NOFOLLOW"}, {AT_REMOVEDIR, "AT_REMOVEDIR"}, {AT_SYMLINK_FOLLOW, "AT_SYMLINK_FOLLOW"}, {AT_NO_AUTOMOUNT, "AT_NO_AUTOMOUNT"}, {AT_EMPTY_PATH, "AT_EMPTY_PATH"}}) {};
};
extern std::shared_ptr<ArionFileATFlagType> ARION_FILE_AT_FLAG_TYPE;

class ArionStatxMaskType : public ArionFlagType
{
  public:
    ArionStatxMaskType() : ArionFlagType("Statx mask", {{STATX_TYPE, "STATX_TYPE"}, {STATX_MODE, "STATX_MODE"}, {STATX_NLINK, "STATX_NLINK"}, {STATX_UID, "STATX_UID"}, {STATX_GID, "STATX_GID"}, {STATX_ATIME, "STATX_ATIME"}, {STATX_MTIME, "STATX_MTIME"}, {STATX_CTIME, "STATX_CTIME"}, {STATX_INO, "STATX_INO"}, {STATX_SIZE, "STATX_SIZE"}, {STATX_BLOCKS, "STATX_BLOCKS"}, {STATX_BASIC_STATS, "STATX_BASIC_STATS"}, {STATX_BTIME, "STATX_BTIME"}, {STATX_ALL, "STATX_ALL"}, {STATX_MNT_ID, "STATX_MNT_ID"}, {0x2000, "STATX_DIOALIGN"}, {0x4000, "STATX_MNT_ID_UNIQUE"}, {0x8000, "STATX_SUBVOL"}, {0x10000, "STATX_WRITE_ATOMIC"}}) {};
};
extern std::shared_ptr<ArionStatxMaskType> ARION_STATX_MASK_TYPE;

class ArionStatxAttrsType : public ArionFlagType
{
  public:
    ArionStatxAttrsType() : ArionFlagType("Statx attributes", {{STATX_ATTR_COMPRESSED, "STATX_ATTR_COMPRESSED"}, {STATX_ATTR_IMMUTABLE, "STATX_ATTR_IMMUTABLE"}, {STATX_ATTR_APPEND, "STATX_ATTR_APPEND"}, {STATX_ATTR_NODUMP, "STATX_ATTR_NODUMP"}, {STATX_ATTR_ENCRYPTED, "STATX_ATTR_ENCRYPTED"}, {STATX_ATTR_VERITY, "STATX_ATTR_VERITY"}, {0x400000, "STATX_ATTR_WRITE_ATOMIC"}, {STATX_ATTR_DAX, "STATX_ATTR_DAX"}, {STATX_ATTR_MOUNT_ROOT, "STATX_ATTR_MOUNT_ROOT"}}) {};
};
extern std::shared_ptr<ArionStatxAttrsType> ARION_STATX_ATTRS_TYPE;

class ArionProtFlagType : public ArionType
{
  public:
    ArionProtFlagType() : ArionType("Protection flag", ARION_LOG_COLOR::ORANGE) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionProtFlagType> ARION_PROT_FLAG_TYPE;

class ArionMmapFlagType : public ArionFlagType
{
  public:
    ArionMmapFlagType() : ArionFlagType("Mmap flag", {{MAP_SHARED, "MAP_SHARED"}, {MAP_SHARED_VALIDATE, "MAP_SHARED_VALIDATE"}, {MAP_PRIVATE, "MAP_PRIVATE"}, {MAP_32BIT, "MAP_32BIT"}, {MAP_ANON, "MAP_ANON"}, {MAP_ANONYMOUS, "MAP_ANONYMOUS"}, {MAP_DENYWRITE, "MAP_DENYWRITE"}, {MAP_EXECUTABLE, "MAP_EXECUTABLE"}, {MAP_FILE, "MAP_FILE"}, {MAP_FIXED, "MAP_FIXED"}, {MAP_FIXED_NOREPLACE, "MAP_FIXED_NOREPLACE"}, {MAP_GROWSDOWN, "MAP_GROWSDOWN"}, {MAP_HUGETLB, "MAP_HUGETLB"}, {MAP_LOCKED, "MAP_LOCKED"}, {MAP_NONBLOCK, "MAP_NONBLOCK"}, {MAP_NORESERVE, "MAP_NORESERVE"}, {MAP_POPULATE, "MAP_POPULATE"}, {MAP_STACK, "MAP_STACK"}, {MAP_SYNC, "MAP_SYNC"}}) {};
};
extern std::shared_ptr<ArionMmapFlagType> ARION_MMAP_FLAG_TYPE;

class ArionSeekWhenceType : public ArionFlagType
{
  public:
    ArionSeekWhenceType() : ArionFlagType("Seek whence", {{SEEK_SET, "SEEK_SET"}, {SEEK_CUR, "SEEK_CUR"}, {SEEK_END, "SEEK_END"}, {SEEK_DATA, "SEEK_DATA"}, {SEEK_HOLE, "SEEK_HOLE"}}) {};
};
extern std::shared_ptr<ArionSeekWhenceType> ARION_SEEK_WHENCE_TYPE;

class ArionFutexOpType : public ArionFlagType
{
  public:
    ArionFutexOpType() : ArionFlagType("Futex operation", {{FUTEX_PRIVATE_FLAG, "FUTEX_PRIVATE_FLAG"}, {FUTEX_CLOCK_REALTIME, "FUTEX_CLOCK_REALTIME"}, {FUTEX_WAIT, "FUTEX_WAIT"}, {FUTEX_WAKE, "FUTEX_WAKE"}, {FUTEX_FD, "FUTEX_FD"}, {FUTEX_REQUEUE, "FUTEX_REQUEUE"}, {FUTEX_CMP_REQUEUE, "FUTEX_CMP_REQUEUE"}, {FUTEX_WAKE_OP, "FUTEX_WAKE_OP"}, {FUTEX_WAIT_BITSET, "FUTEX_WAIT_BITSET"}, {FUTEX_LOCK_PI, "FUTEX_LOCK_PI"}, {FUTEX_LOCK_PI2, "FUTEX_LOCK_PI2"}, {FUTEX_TRYLOCK_PI, "FUTEX_TRYLOCK_PI"}, {FUTEX_UNLOCK_PI, "FUTEX_UNLOCK_PI"}, {FUTEX_WAIT_REQUEUE_PI, "FUTEX_WAIT_REQUEUE_PI"}, {FUTEX_CMP_REQUEUE_PI, "FUTEX_CMP_REQUEUE_PI"}}) {};
};
extern std::shared_ptr<ArionFutexOpType> ARION_FUTEX_OP_TYPE;

class ArionCloneFlagType : public ArionFlagType
{
  public:
    ArionCloneFlagType() : ArionFlagType("Clone flag", {{CLONE_CHILD_CLEARTID, "CLONE_CHILD_CLEARTID"}, {CLONE_CHILD_SETTID, "CLONE_CHILD_SETTID"}, {CLONE_CLEAR_SIGHAND, "CLONE_CLEAR_SIGHAND"}, {CLONE_DETACHED, "CLONE_DETACHED"}, {CLONE_FILES, "CLONE_FILES"}, {CLONE_FS, "CLONE_FS"}, {CLONE_INTO_CGROUP, "CLONE_INTO_CGROUP"}, {CLONE_IO, "CLONE_IO"}, {CLONE_NEWCGROUP, "CLONE_NEWCGROUP"}, {CLONE_NEWIPC, "CLONE_NEWIPC"}, {CLONE_NEWNET, "CLONE_NEWNET"}, {CLONE_NEWNS, "CLONE_NEWNS"}, {CLONE_NEWPID, "CLONE_NEWPID"}, {CLONE_NEWUSER, "CLONE_NEWUSER"}, {CLONE_NEWUTS, "CLONE_NEWUTS"}, {CLONE_PARENT, "CLONE_PARENT"}, {CLONE_PARENT_SETTID, "CLONE_PARENT_SETTID"}, {CLONE_PIDFD, "CLONE_PIDFD"}, {CLONE_PTRACE, "CLONE_PTRACE"}, {CLONE_SETTLS, "CLONE_SETTLS"}, {CLONE_SIGHAND, "CLONE_SIGHAND"}, {CLONE_SYSVSEM, "CLONE_SYSVSEM"}, {CLONE_THREAD, "CLONE_THREAD"}, {CLONE_UNTRACED, "CLONE_UNTRACED"}, {CLONE_VFORK, "CLONE_VFORK"}, {CLONE_VM, "CLONE_VM"}}) {};
};
extern std::shared_ptr<ArionCloneFlagType> ARION_CLONE_FLAG_TYPE;

class ArionSignalType : public ArionFlagType
{
  public:
    ArionSignalType() : ArionFlagType("Signal", {{SIGHUP, "SIGHUP"}, {SIGINT, "SIGINT"}, {SIGQUIT, "SIGQUIT"}, {SIGILL, "SIGILL"}, {SIGTRAP, "SIGTRAP"}, {SIGABRT, "SIGABRT"}, {SIGIOT, "SIGIOT"}, {SIGBUS, "SIGBUS"}, {SIGFPE, "SIGFPE"}, {SIGKILL, "SIGKILL"}, {SIGUSR1, "SIGUSR1"}, {SIGSEGV, "SIGSEGV"}, {SIGUSR2, "SIGUSR2"}, {SIGPIPE, "SIGPIPE"}, {SIGALRM, "SIGALRM"}, {SIGTERM, "SIGTERM"}, {SIGSTKFLT, "SIGSTKFLT"}, {SIGCHLD, "SIGCHLD"}, {SIGCONT, "SIGCONT"}, {SIGSTOP, "SIGSTOP"}, {SIGTSTP, "SIGTSTP"}, {SIGTTIN, "SIGTTIN"}, {SIGTTOU, "SIGTTOU"}, {SIGURG, "SIGURG"}, {SIGXCPU, "SIGXCPU"}, {SIGXFSZ, "SIGXFSZ"}, {SIGVTALRM, "SIGVTALRM"}, {SIGPROF, "SIGPROF"}, {SIGWINCH, "SIGWINCH"}, {SIGIO, "SIGIO"}, {SIGPOLL, "SIGPOLL"}, {SIGPWR, "SIGPWR"}, {SIGSYS, "SIGSYS"}}) {};
};
extern std::shared_ptr<ArionSignalType> ARION_SIGNAL_TYPE;

class ArionFileModeType : public ArionFlagType
{
  public:
    ArionFileModeType() : ArionFlagType("File mode", {{S_IFMT, "S_IFMT"}, {S_IFSOCK, "S_IFSOCK"}, {S_IFLNK, "S_IFLNK"}, {S_IFREG, "S_IFREG"}, {S_IFBLK, "S_IFBLK"}, {S_IFDIR, "S_IFDIR"}, {S_IFCHR, "S_IFCHR"}, {S_IFIFO, "S_IFIFO"}, {S_ISUID, "S_ISUID"}, {S_ISGID, "S_ISGID"}, {S_ISVTX, "S_ISVTX"}, {S_IRWXU, "S_IRWXU"}, {S_IRUSR, "S_IRUSR"}, {S_IWUSR, "S_IWUSR"}, {S_IXUSR, "S_IXUSR"}, {S_IRWXG, "S_IRWXG"}, {S_IRGRP, "S_IRGRP"}, {S_IWGRP, "S_IWGRP"}, {S_IXGRP, "S_IXGRP"}, {S_IRWXO, "S_IRWXO"}, {S_IROTH, "S_IROTH"}, {S_IWOTH, "S_IWOTH"}, {S_IXOTH, "S_IXOTH"}}) {};
};
extern std::shared_ptr<ArionFileModeType> ARION_FILE_MODE_TYPE;

namespace arion_poly_struct
{

class StatStructFactory : public PolymorphicStructFactory<struct stat>
{
  public:
    StatStructFactory()
        : PolymorphicStructFactory({{PTR_SZ, ARION_INT_TYPE, "st_dev"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_ino"},
                                    {{32}, V16, ARION_FILE_MODE_TYPE, "st_mode"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, ARION_FILE_MODE_TYPE, "st_mode"},
                                    {{32}, V16, ARION_INT_TYPE, "st_nlink"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, V64, ARION_INT_TYPE, "st_nlink"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, ARION_INT_TYPE, "st_nlink"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, V32, ARION_FILE_MODE_TYPE, "st_mode"},
                                    {{32}, V16, ARION_INT_TYPE, "st_uid"},
                                    {{64}, V32, ARION_INT_TYPE, "st_uid"},
                                    {{32}, V16, ARION_INT_TYPE, "st_gid"},
                                    {{64}, V32, ARION_INT_TYPE, "st_gid"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_rdev"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, PTR_SZ, ARION_INT_TYPE, "__pad1"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_size"},
                                    {{32}, PTR_SZ, ARION_INT_TYPE, "st_blksize"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, ARION_INT_TYPE, "st_blksize"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, ARION_INT_TYPE, "st_blksize"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, ARION_INT_TYPE, "__pad2"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_blocks"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_atime"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_atime_ns"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_mtime"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_mtime_ns"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_ctime"},
                                    {PTR_SZ, ARION_INT_TYPE, "st_ctime_ns"},
                                    {{32}, PTR_SZ, ARION_INT_TYPE, "__unused1"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, ARION_INT_TYPE, "__unused1"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, ARION_INT_TYPE, "__unused1"},
                                    {{32}, PTR_SZ, ARION_INT_TYPE, "__unused2"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, ARION_INT_TYPE, "__unused2"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, ARION_INT_TYPE, "__unused2"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, ARION_INT_TYPE, "__unused3"}}) {};
};
extern std::shared_ptr<StatStructFactory> STAT_STRUCT_FACTORY;

class ArionStructStatType : public ArionStructType<struct stat>
{
  public:
    ArionStructStatType() : ArionStructType("Struct stat", arion_poly_struct::STAT_STRUCT_FACTORY) {};
};
extern std::shared_ptr<ArionStructStatType> ARION_STRUCT_STAT_TYPE;

class StatxStructFactory : public PolymorphicStructFactory<struct statx>
{
  public:
    StatxStructFactory()
        : PolymorphicStructFactory({{V32, ARION_STATX_MASK_TYPE, "stx_mask"},
                                    {V32, ARION_INT_TYPE, "stx_blksize"},
                                    {PTR_SZ, ARION_STATX_ATTRS_TYPE, "stx_attributes"},
                                    {V32, ARION_INT_TYPE, "stx_nlink"},
                                    {V32, ARION_INT_TYPE, "stx_uid"},
                                    {V32, ARION_INT_TYPE, "stx_gid"},
                                    {V16, ARION_FILE_MODE_TYPE, "stx_mode"},
                                    {V16, ARION_INT_TYPE, "__unused1"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_ino"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_size"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_blocks"},
                                    {PTR_SZ, ARION_STATX_ATTRS_TYPE, "stx_attributes_mask"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_atime_tv_sec"},
                                    {V32, ARION_INT_TYPE, "stx_atime_tv_nsec"},
                                    {V32, ARION_INT_TYPE, "stx_atime_unused1"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_btime_tv_sec"},
                                    {V32, ARION_INT_TYPE, "stx_btime_tv_nsec"},
                                    {V32, ARION_INT_TYPE, "stx_btime_unused1"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_ctime_tv_sec"},
                                    {V32, ARION_INT_TYPE, "stx_ctime_tv_nsec"},
                                    {V32, ARION_INT_TYPE, "stx_ctime_unused1"},
                                    {PTR_SZ, ARION_INT_TYPE, "stx_mtime_tv_sec"},
                                    {V32, ARION_INT_TYPE, "stx_mtime_tv_nsec"},
                                    {V32, ARION_INT_TYPE, "stx_mtime_unused1"},
                                    {V32, ARION_INT_TYPE, "stx_rdev_major"},
                                    {V32, ARION_INT_TYPE, "stx_rdev_minor"},
                                    {V32, ARION_INT_TYPE, "stx_dev_major"},
                                    {V32, ARION_INT_TYPE, "stx_dev_minor"},
                                    {PTR_ARR, ARION_INT_TYPE, "unused2", 14},
                                    }) {};
};
extern std::shared_ptr<StatxStructFactory> STATX_STRUCT_FACTORY;

class ArionStructStatxType : public ArionStructType<struct statx>
{
  public:
    ArionStructStatxType() : ArionStructType("Struct statx", arion_poly_struct::STATX_STRUCT_FACTORY) {};
};
extern std::shared_ptr<ArionStructStatxType> ARION_STRUCT_STATX_TYPE;

class TimespecStructFactory : public PolymorphicStructFactory<struct timespec>
{
  public:
    TimespecStructFactory()
        : PolymorphicStructFactory({{V64, ARION_INT_TYPE, "tv_sec"},
                                    {V64, ARION_INT_TYPE, "tv_nsec"}
                                    }) {};
};
extern std::shared_ptr<TimespecStructFactory> TIMESPEC_STRUCT_FACTORY;

class ArionStructTimespecType : public ArionStructType<struct timespec>
{
  public:
    ArionStructTimespecType() : ArionStructType("Struct timespec", arion_poly_struct::TIMESPEC_STRUCT_FACTORY) {};
};
extern std::shared_ptr<ArionStructTimespecType> ARION_STRUCT_TIMESPEC_TYPE;

class CloneArgsStructFactory : public PolymorphicStructFactory<struct clone_args>
{
  public:
    CloneArgsStructFactory()
        : PolymorphicStructFactory({{V64, ARION_CLONE_FLAG_TYPE, "flags"},
                                    {V64, ARION_INT_TYPE, "pidfd"},
                                    {V64, ARION_INT_TYPE, "child_tid"},
                                    {V64, ARION_INT_TYPE, "parent_tid"},
                                    {V64, ARION_SIGNAL_TYPE, "exit_signal"},
                                    {V64, ARION_INT_TYPE, "stack"},
                                    {V64, ARION_INT_TYPE, "stack_size"},
                                    {V64, ARION_INT_TYPE, "tls"},
                                    {V64, ARION_INT_TYPE, "set_tid"},
                                    {V64, ARION_INT_TYPE, "set_tid_size"},
                                    {V64, ARION_FILE_DESCRIPTOR_TYPE, "cgroup"}
                                    }) {};
};
extern std::shared_ptr<CloneArgsStructFactory> CLONE_ARGS_STRUCT_FACTORY;

class ArionStructCloneArgsType : public ArionStructType<struct clone_args>
{
  public:
    ArionStructCloneArgsType() : ArionStructType("Struct clone_args", arion_poly_struct::CLONE_ARGS_STRUCT_FACTORY) {};
};
extern std::shared_ptr<ArionStructCloneArgsType> ARION_STRUCT_CLONE_ARGS_TYPE;
} // namespace arion_poly_struct

#endif // ARION_LNX_KERNEL_UTILS_HPP
