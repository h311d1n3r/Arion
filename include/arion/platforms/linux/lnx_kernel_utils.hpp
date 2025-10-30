#ifndef ARION_LNX_KERNEL_UTILS_HPP
#define ARION_LNX_KERNEL_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/utils/struct_utils.hpp>
#include <arion/utils/type_utils.hpp>
#include <fcntl.h>
#include <linux/futex.h>
#include <linux/sched.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/un.h>

namespace arion_lnx_type
{

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

struct __kernel_old_timeval
{
    __kernel_long_t tv_sec;
    __kernel_long_t tv_usec;
};

struct elf_siginfo
{
    int si_signo;
    int si_code;
    int si_errno;
};

struct elf_prstatus_common
{
    struct elf_siginfo pr_info;
    short pr_cursig;
    unsigned long pr_sigpend;
    unsigned long pr_sighold;
    pid_t pr_pid;
    pid_t pr_ppid;
    pid_t pr_pgrp;
    pid_t pr_sid;
    struct __kernel_old_timeval pr_utime;
    struct __kernel_old_timeval pr_stime;
    struct __kernel_old_timeval pr_cutime;
    struct __kernel_old_timeval pr_cstime;
};

typedef unsigned long elf_greg_t;
typedef unsigned long elf_freg_t[3];

arion::PROT_FLAGS kernel_prot_to_arion_prot(int kflags);

class ErrCodeType : public arion_type::KernelType
{
  public:
    ErrCodeType() : arion_type::KernelType("Error code", arion::LOG_COLOR::MAGENTA) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ErrCodeType> ERR_CODE_TYPE;

class FileDescriptorType : public arion_type::KernelType
{
  public:
    FileDescriptorType() : arion_type::KernelType("File descriptor", arion::LOG_COLOR::MAGENTA) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<FileDescriptorType> FILE_DESCRIPTOR_TYPE;

class InAddrTType : public arion_type::KernelType
{
  public:
    InAddrTType() : arion_type::KernelType("Type in_addr_t", arion::LOG_COLOR::CYAN) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<InAddrTType> IN_ADDR_T_TYPE;

class In6AddrTType : public arion_type::KernelType
{
  public:
    In6AddrTType() : arion_type::KernelType("Type in6_addr_t", arion::LOG_COLOR::CYAN) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<In6AddrTType> IN6_ADDR_T_TYPE;

class AccessModeType : public arion_type::FlagType
{
  public:
    AccessModeType() : arion_type::FlagType("Access mode", {{0, "F_OK"}, {1, "X_OK"}, {2, "W_OK"}, {4, "R_OK"}}) {};
};
extern std::shared_ptr<AccessModeType> ACCESS_MODE_TYPE;

class OpenModeType : public arion_type::FlagType
{
  public:
    OpenModeType()
        : arion_type::FlagType("Open mode", {{0, "O_RDONLY"},
                                             {1, "O_WRONLY"},
                                             {2, "O_RDWR"},
                                             {0100, "O_CREAT"},
                                             {0200, "O_EXCL"},
                                             {0400, "O_NOCTTY"},
                                             {01000, "O_TRUNC"},
                                             {02000, "O_APPEND"},
                                             {04000, "O_NONBLOCK"},
                                             {04010000, "O_SYNC"},
                                             {020000, "O_ASYNC"},
                                             {0100000, "O_LARGEFILE"},
                                             {0200000, "O_DIRECTORY"},
                                             {0400000, "O_NOFOLLOW"},
                                             {02000000, "O_CLOEXEC"},
                                             {040000, "O_DIRECT"},
                                             {01000000, "O_NOATIME"},
                                             {010000000, "O_PATH"},
                                             {010000, "O_DSYNC"},
                                             {020000000 | 0200000, "__O_TMPFILE"}}) {};
};
extern std::shared_ptr<OpenModeType> OPEN_MODE_TYPE;

class FileATFlagType : public arion_type::FlagType
{
  public:
    FileATFlagType()
        : arion_type::FlagType("File AT flag", {{-100, "AT_FDCWD"},
                                                {0x100, "AT_SYMLINK_NOFOLLOW"},
                                                {0x200, "AT_REMOVEDIR"},
                                                {0x400, "AT_SYMLINK_FOLLOW"},
                                                {0x800, "AT_NO_AUTOMOUNT"},
                                                {0x1000, "AT_EMPTY_PATH"},
                                                {0x6000, "AT_STATX_SYNC_TYPE"},
                                                {0x0000, "AT_STATX_SYNC_AS_STAT"},
                                                {0x2000, "AT_STATX_FORCE_SYNC"},
                                                {0x4000, "AT_STATX_DONT_SYNC"},
                                                {0x8000, "AT_RECURSIVE"}}) {};
};
extern std::shared_ptr<FileATFlagType> FILE_AT_FLAG_TYPE;

class StatxMaskType : public arion_type::FlagType
{
  public:
    StatxMaskType()
        : arion_type::FlagType("Statx mask", {{0x00000001U, "STATX_TYPE"},
                                              {0x00000002U, "STATX_MODE"},
                                              {0x00000004U, "STATX_NLINK"},
                                              {0x00000008U, "STATX_UID"},
                                              {0x00000010U, "STATX_GID"},
                                              {0x00000020U, "STATX_ATIME"},
                                              {0x00000040U, "STATX_MTIME"},
                                              {0x00000080U, "STATX_CTIME"},
                                              {0x00000100U, "STATX_INO"},
                                              {0x00000200U, "STATX_SIZE"},
                                              {0x00000400U, "STATX_BLOCKS"},
                                              {0x000007ffU, "STATX_BASIC_STATS"},
                                              {0x00000800U, "STATX_BTIME"},
                                              {0x00001000U, "STATX_MNT_ID"},
                                              {0x00002000U, "STATX_DIOALIGN"},
                                              {0x00004000U, "STATX_MNT_ID_UNIQUE"},
                                              {0x00008000U, "STATX_SUBVOL"},
                                              {0x10000, "STATX_WRITE_ATOMIC"}}) {};
};
extern std::shared_ptr<StatxMaskType> STATX_MASK_TYPE;

class StatxAttrsType : public arion_type::FlagType
{
  public:
    StatxAttrsType()
        : arion_type::FlagType("Statx attributes", {{0x4, "STATX_ATTR_COMPRESSED"},
                                                    {0x10, "STATX_ATTR_IMMUTABLE"},
                                                    {0x20, "STATX_ATTR_APPEND"},
                                                    {0x40, "STATX_ATTR_NODUMP"},
                                                    {0x800, "STATX_ATTR_ENCRYPTED"},
                                                    {0x1000, "STATX_ATTR_VERITY"},
                                                    {0x2000, "STATX_ATTR_MOUNT_ROOT"},
                                                    {0x200000, "STATX_ATTR_DAX"},
                                                    {0x400000, "STATX_ATTR_WRITE_ATOMIC"}}) {};
};
extern std::shared_ptr<StatxAttrsType> STATX_ATTRS_TYPE;

class ProtFlagType : public arion_type::KernelType
{
  public:
    ProtFlagType() : arion_type::KernelType("Protection flag", arion::LOG_COLOR::ORANGE) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ProtFlagType> PROT_FLAG_TYPE;

class MmapFlagType : public arion_type::FlagType
{
  public:
    MmapFlagType()
        : arion_type::FlagType("Mmap flag", {{0x0, "MAP_FILE"},
                                             {0x1, "MAP_SHARED"},
                                             {0x2, "MAP_PRIVATE"},
                                             {0x3, "MAP_SHARED_VALIDATE"},
                                             {0x10, "MAP_FIXED"},
                                             {0x20, "MAP_ANONYMOUS"},
                                             {0x40, "MAP_32BIT"},
                                             {0x100, "MAP_GROWSDOWN"},
                                             {0x800, "MAP_DENYWRITE"},
                                             {0x1000, "MAP_EXECUTABLE"},
                                             {0x2000, "MAP_LOCKED"},
                                             {0x4000, "MAP_NORESERVE"},
                                             {0x8000, "MAP_POPULATE"},
                                             {0x10000, "MAP_NONBLOCK"},
                                             {0x20000, "MAP_STACK"},
                                             {0x40000, "MAP_HUGETLB"},
                                             {0x80000, "MAP_SYNC"},
                                             {0x100000, "MAP_FIXED_NOREPLACE"}}) {};
};
extern std::shared_ptr<MmapFlagType> MMAP_FLAG_TYPE;

class SeekWhenceType : public arion_type::FlagType
{
  public:
    SeekWhenceType()
        : arion_type::FlagType(
              "Seek whence", {{0, "SEEK_SET"}, {1, "SEEK_CUR"}, {2, "SEEK_END"}, {3, "SEEK_DATA"}, {4, "SEEK_HOLE"}}) {
          };
};
extern std::shared_ptr<SeekWhenceType> SEEK_WHENCE_TYPE;

class FutexOpType : public arion_type::FlagType
{
  public:
    FutexOpType()
        : arion_type::FlagType("Futex operation", {{0, "FUTEX_WAIT"},
                                                   {1, "FUTEX_WAKE"},
                                                   {2, "FUTEX_FD"},
                                                   {3, "FUTEX_REQUEUE"},
                                                   {4, "FUTEX_CMP_REQUEUE"},
                                                   {5, "FUTEX_WAKE_OP"},
                                                   {6, "FUTEX_LOCK_PI"},
                                                   {7, "FUTEX_UNLOCK_PI"},
                                                   {8, "FUTEX_TRYLOCK_PI"},
                                                   {9, "FUTEX_WAIT_BITSET"},
                                                   {10, "FUTEX_WAKE_BITSET"},
                                                   {11, "FUTEX_WAIT_REQUEUE_PI"},
                                                   {12, "FUTEX_CMP_REQUEUE_PI"},
                                                   {13, "FUTEX_LOCK_PI2"},
                                                   {128, "FUTEX_PRIVATE_FLAG"},
                                                   {256, "FUTEX_CLOCK_REALTIME"},
                                                   // Private variants (base + FUTEX_PRIVATE_FLAG = +128)
                                                   {0 + 128, "FUTEX_WAIT_PRIVATE"},
                                                   {1 + 128, "FUTEX_WAKE_PRIVATE"},
                                                   {3 + 128, "FUTEX_REQUEUE_PRIVATE"},
                                                   {4 + 128, "FUTEX_CMP_REQUEUE_PRIVATE"},
                                                   {5 + 128, "FUTEX_WAKE_OP_PRIVATE"},
                                                   {6 + 128, "FUTEX_LOCK_PI_PRIVATE"},
                                                   {13 + 128, "FUTEX_LOCK_PI2_PRIVATE"},
                                                   {7 + 128, "FUTEX_UNLOCK_PI_PRIVATE"},
                                                   {8 + 128, "FUTEX_TRYLOCK_PI_PRIVATE"},
                                                   {9 + 128, "FUTEX_WAIT_BITSET_PRIVATE"},
                                                   {10 + 128, "FUTEX_WAKE_BITSET_PRIVATE"},
                                                   {11 + 128, "FUTEX_WAIT_REQUEUE_PI_PRIVATE"},
                                                   {12 + 128, "FUTEX_CMP_REQUEUE_PI_PRIVATE"}}) {};
};
extern std::shared_ptr<FutexOpType> FUTEX_OP_TYPE;

class CloneFlagType : public arion_type::FlagType
{
  public:
    CloneFlagType()
        : arion_type::FlagType("Clone flag", {{0x000000ff, "CSIGNAL"},
                                              {0x00000100, "CLONE_VM"},
                                              {0x00000200, "CLONE_FS"},
                                              {0x00000400, "CLONE_FILES"},
                                              {0x00000800, "CLONE_SIGHAND"},
                                              {0x00001000, "CLONE_PIDFD"},
                                              {0x00002000, "CLONE_PTRACE"},
                                              {0x00004000, "CLONE_VFORK"},
                                              {0x00008000, "CLONE_PARENT"},
                                              {0x00010000, "CLONE_THREAD"},
                                              {0x00020000, "CLONE_NEWNS"},
                                              {0x00040000, "CLONE_SYSVSEM"},
                                              {0x00080000, "CLONE_SETTLS"},
                                              {0x00100000, "CLONE_PARENT_SETTID"},
                                              {0x00200000, "CLONE_CHILD_CLEARTID"},
                                              {0x00400000, "CLONE_DETACHED"},
                                              {0x00800000, "CLONE_UNTRACED"},
                                              {0x01000000, "CLONE_CHILD_SETTID"},
                                              {0x02000000, "CLONE_NEWCGROUP"},
                                              {0x04000000, "CLONE_NEWUTS"},
                                              {0x08000000, "CLONE_NEWIPC"},
                                              {0x10000000, "CLONE_NEWUSER"},
                                              {0x20000000, "CLONE_NEWPID"},
                                              {0x40000000, "CLONE_NEWNET"},
                                              {0x80000000, "CLONE_IO"},
                                              {0x100000000ULL, "CLONE_CLEAR_SIGHAND"},
                                              {0x200000000ULL, "CLONE_INTO_CGROUP"},
                                              {0x00000080, "CLONE_NEWTIME"}}) {};
};
extern std::shared_ptr<CloneFlagType> CLONE_FLAG_TYPE;

class SignalType : public arion_type::FlagType
{
  public:
    SignalType()
        : arion_type::FlagType("Signal",
                               {{1, "SIGHUP"},   {2, "SIGINT"},     {3, "SIGQUIT"},  {4, "SIGILL"},    {5, "SIGTRAP"},
                                {6, "SIGABRT"},  {6, "SIGIOT"},     {7, "SIGBUS"},   {8, "SIGFPE"},    {9, "SIGKILL"},
                                {10, "SIGUSR1"}, {11, "SIGSEGV"},   {12, "SIGUSR2"}, {13, "SIGPIPE"},  {14, "SIGALRM"},
                                {15, "SIGTERM"}, {16, "SIGSTKFLT"}, {17, "SIGCHLD"}, {18, "SIGCONT"},  {19, "SIGSTOP"},
                                {20, "SIGTSTP"}, {21, "SIGTTIN"},   {22, "SIGTTOU"}, {23, "SIGURG"},   {24, "SIGXCPU"},
                                {25, "SIGXFSZ"}, {26, "SIGVTALRM"}, {27, "SIGPROF"}, {28, "SIGWINCH"}, {29, "SIGIO"},
                                {30, "SIGPWR"},  {31, "SIGSYS"}}) {};
};
extern std::shared_ptr<SignalType> SIGNAL_TYPE;

class FileModeType : public arion_type::FlagType
{
  public:
    FileModeType()
        : arion_type::FlagType("File mode",
                               {{00170000, "S_IFMT"}, {0140000, "S_IFSOCK"}, {0120000, "S_IFLNK"}, {0100000, "S_IFREG"},
                                {0060000, "S_IFBLK"}, {0040000, "S_IFDIR"},  {0020000, "S_IFCHR"}, {0010000, "S_IFIFO"},
                                {0004000, "S_ISUID"}, {0002000, "S_ISGID"},  {0001000, "S_ISVTX"}, {00700, "S_IRWXU"},
                                {00400, "S_IRUSR"},   {00200, "S_IWUSR"},    {00100, "S_IXUSR"},   {00070, "S_IRWXG"},
                                {00040, "S_IRGRP"},   {00020, "S_IWGRP"},    {00010, "S_IXGRP"},   {00007, "S_IRWXO"},
                                {00004, "S_IROTH"},   {00002, "S_IWOTH"},    {00001, "S_IXOTH"}}) {};
};
extern std::shared_ptr<FileModeType> FILE_MODE_TYPE;

class SocketDomainType : public arion_type::FlagType
{
  public:
    SocketDomainType()
        : arion_type::FlagType(
              "Socket domain",
              {{0, "AF_UNSPEC"},    {1, "AF_LOCAL"},       {2, "AF_INET"},     {3, "AF_AX25"},     {4, "AF_IPX"},
               {5, "AF_APPLETALK"}, {6, "AF_NETROM"},      {7, "AF_BRIDGE"},   {8, "AF_ATMPVC"},   {9, "AF_X25"},
               {10, "AF_INET6"},    {11, "AF_ROSE"},       {12, "AF_DECnet"},  {13, "AF_NETBEUI"}, {14, "AF_SECURITY"},
               {15, "AF_KEY"},      {16, "AF_ROUTE"},      {17, "AF_PACKET"},  {18, "AF_ASH"},     {19, "AF_ECONET"},
               {20, "AF_ATMSVC"},   {21, "AF_RDS"},        {22, "AF_SNA"},     {23, "AF_IRDA"},    {24, "AF_PPPOX"},
               {25, "AF_WANPIPE"},  {26, "AF_LLC"},        {27, "AF_IB"},      {28, "AF_MPLS"},    {29, "AF_CAN"},
               {30, "AF_TIPC"},     {31, "AF_BLUETOOTH"},  {32, "AF_IUCV"},    {33, "AF_RXRPC"},   {34, "AF_ISDN"},
               {35, "AF_PHONET"},   {36, "AF_IEEE802154"}, {37, "AF_CAIF"},    {38, "AF_ALG"},     {39, "AF_NFC"},
               {40, "AF_VSOCK"},    {41, "AF_KCM"},        {42, "AF_QIPCRTR"}, {43, "AF_SMC"},     {44, "AF_XDP"},
               {45, "AF_MCTP"}}) {};
};
extern std::shared_ptr<SocketDomainType> SOCKET_DOMAIN_TYPE;

class SocketTypeType : public arion_type::FlagType
{
  public:
    SocketTypeType()
        : arion_type::FlagType("Socket domain", {{1, "SOCK_STREAM"},
                                                 {2, "SOCK_DGRAM"},
                                                 {3, "SOCK_RAW"},
                                                 {4, "SOCK_RDM"},
                                                 {5, "SOCK_SEQPACKET"},
                                                 {6, "SOCK_DCCP"},
                                                 {10, "SOCK_PACKET"},
                                                 {00004000, "SOCK_NONBLOCK"},
                                                 {02000000, "SOCK_CLOEXEC"}}) {};
};
extern std::shared_ptr<SocketTypeType> SOCKET_TYPE_TYPE;

class StatStructFactory : public arion_poly_struct::PolymorphicStructFactory<struct stat>
{
  public:
    StatStructFactory()
        : PolymorphicStructFactory(
              {{ftype::PTR_SZ, arion_type::INT_TYPE, "st_dev"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_ino"},
               {{32}, ftype::V16, arion_lnx_type::FILE_MODE_TYPE, "st_mode"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::V32, arion_lnx_type::FILE_MODE_TYPE, "st_mode"},
               {{32}, ftype::V16, arion_type::INT_TYPE, "st_nlink"},
               {{arion::CPU_ARCH::X8664_ARCH}, ftype::V64, arion_type::INT_TYPE, "st_nlink"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::V32, arion_type::INT_TYPE, "st_nlink"},
               {{arion::CPU_ARCH::X8664_ARCH}, ftype::V32, arion_lnx_type::FILE_MODE_TYPE, "st_mode"},
               {{32}, ftype::V16, arion_type::INT_TYPE, "st_uid"},
               {{64}, ftype::V32, arion_type::INT_TYPE, "st_uid"},
               {{32}, ftype::V16, arion_type::INT_TYPE, "st_gid"},
               {{64}, ftype::V32, arion_type::INT_TYPE, "st_gid"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_rdev"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::PTR_SZ, arion_type::INT_TYPE, "__pad1"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_size"},
               {{32}, ftype::PTR_SZ, arion_type::INT_TYPE, "st_blksize"},
               {{arion::CPU_ARCH::X8664_ARCH}, ftype::PTR_SZ, arion_type::INT_TYPE, "st_blksize"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::V32, arion_type::INT_TYPE, "st_blksize"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::V32, arion_type::INT_TYPE, "__pad2"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_blocks"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_atime"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_atime_ns"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_mtime"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_mtime_ns"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_ctime"},
               {ftype::PTR_SZ, arion_type::INT_TYPE, "st_ctime_ns"},
               {{32}, ftype::PTR_SZ, arion_type::INT_TYPE, "__unused1"},
               {{arion::CPU_ARCH::X8664_ARCH}, ftype::PTR_SZ, arion_type::INT_TYPE, "__unused1"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::V32, arion_type::INT_TYPE, "__unused1"},
               {{32}, ftype::PTR_SZ, arion_type::INT_TYPE, "__unused2"},
               {{arion::CPU_ARCH::X8664_ARCH}, ftype::PTR_SZ, arion_type::INT_TYPE, "__unused2"},
               {{arion::CPU_ARCH::ARM64_ARCH}, ftype::V32, arion_type::INT_TYPE, "__unused2"},
               {{arion::CPU_ARCH::X8664_ARCH}, ftype::PTR_SZ, arion_type::INT_TYPE, "__unused3"}}) {};
};
extern std::shared_ptr<StatStructFactory> STAT_STRUCT_FACTORY;

class StructStatType : public arion_poly_struct::ArionStructType<struct stat>
{
  public:
    StructStatType() : ArionStructType("Struct stat", STAT_STRUCT_FACTORY) {};
};
extern std::shared_ptr<StructStatType> STRUCT_STAT_TYPE;

class StatxStructFactory : public arion_poly_struct::PolymorphicStructFactory<struct statx>
{
  public:
    StatxStructFactory()
        : PolymorphicStructFactory({
              {ftype::V32, arion_lnx_type::STATX_MASK_TYPE, "stx_mask"},
              {ftype::V32, arion_type::INT_TYPE, "stx_blksize"},
              {ftype::V64, arion_lnx_type::STATX_ATTRS_TYPE, "stx_attributes"},
              {ftype::V32, arion_type::INT_TYPE, "stx_nlink"},
              {ftype::V32, arion_type::INT_TYPE, "stx_uid"},
              {ftype::V32, arion_type::INT_TYPE, "stx_gid"},
              {ftype::V16, arion_lnx_type::FILE_MODE_TYPE, "stx_mode"},
              {ftype::V16, arion_type::INT_TYPE, "__unused1"},
              {ftype::V64, arion_type::INT_TYPE, "stx_ino"},
              {ftype::V64, arion_type::INT_TYPE, "stx_size"},
              {ftype::V64, arion_type::INT_TYPE, "stx_blocks"},
              {ftype::V64, arion_lnx_type::STATX_ATTRS_TYPE, "stx_attributes_mask"},
              {ftype::V64, arion_type::INT_TYPE, "stx_atime_tv_sec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_atime_tv_nsec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_atime_unused1"},
              {ftype::V64, arion_type::INT_TYPE, "stx_btime_tv_sec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_btime_tv_nsec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_btime_unused1"},
              {ftype::V64, arion_type::INT_TYPE, "stx_ctime_tv_sec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_ctime_tv_nsec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_ctime_unused1"},
              {ftype::V64, arion_type::INT_TYPE, "stx_mtime_tv_sec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_mtime_tv_nsec"},
              {ftype::V32, arion_type::INT_TYPE, "stx_mtime_unused1"},
              {ftype::V32, arion_type::INT_TYPE, "stx_rdev_major"},
              {ftype::V32, arion_type::INT_TYPE, "stx_rdev_minor"},
              {ftype::V32, arion_type::INT_TYPE, "stx_dev_major"},
              {ftype::V32, arion_type::INT_TYPE, "stx_dev_minor"},
              {ftype::A64, arion_type::INT_TYPE, "unused2", 14},
          }) {};
};
extern std::shared_ptr<StatxStructFactory> STATX_STRUCT_FACTORY;

class StructStatxType : public arion_poly_struct::ArionStructType<struct statx>
{
  public:
    StructStatxType() : ArionStructType("Struct statx", STATX_STRUCT_FACTORY) {};
};
extern std::shared_ptr<StructStatxType> STRUCT_STATX_TYPE;

class TimespecStructFactory : public arion_poly_struct::PolymorphicStructFactory<struct timespec>
{
  public:
    TimespecStructFactory()
        : PolymorphicStructFactory({{ftype::V64, arion_type::INT_TYPE, "tv_sec"},
                                    {ftype::V64, arion_type::INT_TYPE, "tv_nsec"}}) {};
};
extern std::shared_ptr<TimespecStructFactory> TIMESPEC_STRUCT_FACTORY;

class StructTimespecType : public arion_poly_struct::ArionStructType<struct timespec>
{
  public:
    StructTimespecType() : ArionStructType("Struct timespec", TIMESPEC_STRUCT_FACTORY) {};
};
extern std::shared_ptr<StructTimespecType> STRUCT_TIMESPEC_TYPE;

class CloneArgsStructFactory : public arion_poly_struct::PolymorphicStructFactory<struct clone_args>
{
  public:
    CloneArgsStructFactory()
        : PolymorphicStructFactory({{ftype::V64, arion_lnx_type::CLONE_FLAG_TYPE, "flags"},
                                    {ftype::V64, arion_type::INT_TYPE, "pidfd"},
                                    {ftype::V64, arion_type::INT_TYPE, "child_tid"},
                                    {ftype::V64, arion_type::INT_TYPE, "parent_tid"},
                                    {ftype::V64, arion_lnx_type::SIGNAL_TYPE, "exit_signal"},
                                    {ftype::V64, arion_type::INT_TYPE, "stack"},
                                    {ftype::V64, arion_type::INT_TYPE, "stack_size"},
                                    {ftype::V64, arion_type::INT_TYPE, "tls"},
                                    {ftype::V64, arion_type::INT_TYPE, "set_tid"},
                                    {ftype::V64, arion_type::INT_TYPE, "set_tid_size"},
                                    {ftype::V64, arion_lnx_type::FILE_DESCRIPTOR_TYPE, "cgroup"}}) {};
};
extern std::shared_ptr<CloneArgsStructFactory> CLONE_ARGS_STRUCT_FACTORY;

class StructCloneArgsType : public arion_poly_struct::ArionStructType<struct clone_args>
{
  public:
    StructCloneArgsType() : ArionStructType("Struct clone_args", CLONE_ARGS_STRUCT_FACTORY) {};
};
extern std::shared_ptr<StructCloneArgsType> STRUCT_CLONE_ARGS_TYPE;

class SockaddrInStructFactory : public arion_poly_struct::PolymorphicStructFactory<struct sockaddr_in>
{
  public:
    SockaddrInStructFactory()
        : PolymorphicStructFactory({{ftype::V16, arion_lnx_type::SOCKET_DOMAIN_TYPE, "sin_family"},
                                    {ftype::V16, arion_type::INT_TYPE, "sin_port"},
                                    {ftype::V32, arion_lnx_type::IN_ADDR_T_TYPE, "sin_addr"}}) {};
};
extern std::shared_ptr<SockaddrInStructFactory> SOCKADDR_IN_STRUCT_FACTORY;

class StructSockaddrInType : public arion_poly_struct::ArionStructType<struct sockaddr_in>
{
  public:
    StructSockaddrInType() : ArionStructType("Struct sockaddr_in", SOCKADDR_IN_STRUCT_FACTORY) {};
};
extern std::shared_ptr<StructSockaddrInType> STRUCT_SOCKADDR_IN_TYPE;

class SockaddrIn6StructFactory : public arion_poly_struct::PolymorphicStructFactory<struct sockaddr_in6>
{
  public:
    SockaddrIn6StructFactory()
        : PolymorphicStructFactory({
              {ftype::V16, arion_lnx_type::SOCKET_DOMAIN_TYPE, "sin6_family"},
              {ftype::V16, arion_type::INT_TYPE, "sin6_port"},
              {ftype::V32, arion_type::INT_TYPE, "sin6_flowinfo"},
              {ftype::A8, arion_lnx_type::IN6_ADDR_T_TYPE, "sin6_addr", 16},
              {ftype::V32, arion_type::INT_TYPE, "sin6_scope_id"},
          }) {};
};
extern std::shared_ptr<SockaddrIn6StructFactory> SOCKADDR_IN6_STRUCT_FACTORY;

class StructSockaddrIn6Type : public arion_poly_struct::ArionStructType<struct sockaddr_in6>
{
  public:
    StructSockaddrIn6Type() : ArionStructType("Struct sockaddr_in6", SOCKADDR_IN6_STRUCT_FACTORY) {};
};
extern std::shared_ptr<StructSockaddrIn6Type> STRUCT_SOCKADDR_IN6_TYPE;

class SockaddrUnStructFactory : public arion_poly_struct::PolymorphicStructFactory<struct sockaddr_un>
{
  public:
    SockaddrUnStructFactory()
        : PolymorphicStructFactory({{ftype::V16, arion_lnx_type::SOCKET_DOMAIN_TYPE, "sun_family"}}) {};
};
extern std::shared_ptr<SockaddrUnStructFactory> SOCKADDR_UN_STRUCT_FACTORY;

class StructSockaddrUnType : public arion_poly_struct::ArionStructType<struct sockaddr_un>
{
  public:
    StructSockaddrUnType() : ArionStructType("Struct sockaddr_un", SOCKADDR_UN_STRUCT_FACTORY) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<StructSockaddrUnType> STRUCT_SOCKADDR_UN_TYPE;

class StructSockaddrType : public arion_poly_struct::ArionVariableStructType
{
  private:
    std::shared_ptr<arion_poly_struct::AbsArionStructType> process(std::shared_ptr<arion::Arion> arion,
                                                                   uint64_t val) override;

  public:
    StructSockaddrType() : ArionVariableStructType("Struct sockaddr") {};
};
extern std::shared_ptr<StructSockaddrType> STRUCT_SOCKADDR_TYPE;

} // namespace arion_lnx_type

#endif // ARION_LNX_KERNEL_UTILS_HPP
