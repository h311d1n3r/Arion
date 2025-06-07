#ifndef ARION_LNX_KERNEL_UTILS_HPP
#define ARION_LNX_KERNEL_UTILS_HPP

#include <arion/utils/type_utils.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/utils/struct_utils.hpp>
#include <sys/stat.h>
#include <fcntl.h>

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

namespace arion_poly_struct
{

class StatStructFactory : public PolymorphicStructFactory<struct stat>
{
  public:
    StatStructFactory()
        : PolymorphicStructFactory({{PTR_SZ, "st_dev"},
                                    {PTR_SZ, "st_ino"},
                                    {{32}, V16, "st_mode"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, "st_mode"},
                                    {{32}, V16, "st_nlink"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, V64, "st_nlink"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, "st_nlink"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, V32, "st_mode"},
                                    {{32}, V16, "st_uid"},
                                    {{64}, V32, "st_uid"},
                                    {{32}, V16, "st_gid"},
                                    {{64}, V32, "st_gid"},
                                    {PTR_SZ, "st_rdev"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, PTR_SZ, "__pad1"},
                                    {PTR_SZ, "st_size"},
                                    {{32}, PTR_SZ, "st_blksize"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, "st_blksize"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, "st_blksize"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, "__pad2"},
                                    {PTR_SZ, "st_blocks"},
                                    {PTR_SZ, "st_atime"},
                                    {PTR_SZ, "st_atime_ns"},
                                    {PTR_SZ, "st_mtime"},
                                    {PTR_SZ, "st_mtime_ns"},
                                    {PTR_SZ, "st_ctime"},
                                    {PTR_SZ, "st_ctime_ns"},
                                    {{32}, PTR_SZ, "__unused1"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, "__unused1"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, "__unused1"},
                                    {{32}, PTR_SZ, "__unused2"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, "__unused2"},
                                    {{arion::CPU_ARCH::ARM64_ARCH}, V32, "__unused2"},
                                    {{arion::CPU_ARCH::X8664_ARCH}, PTR_SZ, "__unused3"}}) {};
};

extern std::shared_ptr<StatStructFactory> STAT_STRUCT_FACTORY;

class StatxStructFactory : public PolymorphicStructFactory<struct statx>
{
  public:
    StatxStructFactory()
        : PolymorphicStructFactory({{V32, "stx_mask"},
                                    {V32, "stx_blksize"},
                                    {PTR_SZ, "stx_attributes"},
                                    {V32, "stx_nlink"},
                                    {V32, "stx_uid"},
                                    {V32, "stx_gid"},
                                    {V16, "stx_mode"},
                                    {V16, "__unused1"},
                                    {PTR_SZ, "stx_ino"},
                                    {PTR_SZ, "stx_size"},
                                    {PTR_SZ, "stx_blocks"},
                                    {PTR_SZ, "stx_attributes_mask"},
                                    {PTR_SZ, "stx_atime_tv_sec"},
                                    {V32, "stx_atime_tv_nsec"},
                                    {V32, "stx_atime_unused1"},
                                    {PTR_SZ, "stx_btime_tv_sec"},
                                    {V32, "stx_btime_tv_nsec"},
                                    {V32, "stx_btime_unused1"},
                                    {PTR_SZ, "stx_ctime_tv_sec"},
                                    {V32, "stx_ctime_tv_nsec"},
                                    {V32, "stx_ctime_unused1"},
                                    {PTR_SZ, "stx_mtime_tv_sec"},
                                    {V32, "stx_mtime_tv_nsec"},
                                    {V32, "stx_mtime_unused1"},
                                    {V32, "stx_rdev_major"},
                                    {V32, "stx_rdev_minor"},
                                    {V32, "stx_dev_major"},
                                    {V32, "stx_dev_minor"},
                                    {PTR_ARR, "unused2", 14},
                                    }) {};
};

extern std::shared_ptr<StatxStructFactory> STATX_STRUCT_FACTORY;
} // namespace arion_poly_struct

class ArionErrCodeType : public ArionType {
public:
    ArionErrCodeType() : ArionType("Error Code", ARION_LOG_COLOR::MAGENTA) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionErrCodeType> ARION_ERR_CODE_TYPE;

class ArionFileDescriptorType : public ArionType {
public:
    ArionFileDescriptorType() : ArionType("File Descriptor", ARION_LOG_COLOR::MAGENTA) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionFileDescriptorType> ARION_FILE_DESCRIPTOR_TYPE;

class ArionAccessModeType : public ArionFlagType
{
  public:
    ArionAccessModeType() : ArionFlagType("Access Mode", {{F_OK, "F_OK"}, {R_OK, "R_OK"}, {W_OK, "W_OK"}, {X_OK, "X_OK"}}) {};
};
extern std::shared_ptr<ArionAccessModeType> ARION_ACCESS_MODE_TYPE;

class ArionOpenModeType : public ArionFlagType
{
  public:
    ArionOpenModeType() : ArionFlagType("Open Mode", {{O_RDONLY, "O_RDONLY"}, {O_WRONLY, "O_WRONLY"}, {0x4, "O_EXEC"}, {O_RDWR, "O_RDWR"},
                                                        {O_ACCMODE, "O_ACCMODE"}, {O_LARGEFILE, "O_LARGEFILE"}, {O_CREAT, "O_CREAT"}, {O_APPEND, "O_APPEND"},
                                                        {O_CLOEXEC, "O_CLOEXEC"}, {O_DIRECTORY, "O_DIRECTORY"}, {O_DSYNC, "O_DSYNC"}, {O_EXCL, "O_EXCL"},
                                                        {O_NOCTTY, "O_NOCTTY"}, {O_NOFOLLOW, "O_NOFOLLOW"}, {O_NONBLOCK, "O_NONBLOCK"}, {O_RSYNC, "O_RSYNC"},
                                                        {O_SYNC, "O_SYNC"}, {O_TRUNC, "O_TRUNC"}}) {};
};
extern std::shared_ptr<ArionOpenModeType> ARION_OPEN_MODE_TYPE;

class ArionStructStatType : public ArionStructType<struct stat>
{
  public:
    ArionStructStatType() : ArionStructType("Struct stat", arion_poly_struct::STAT_STRUCT_FACTORY) {};
};
extern std::shared_ptr<ArionStructStatType> ARION_STRUCT_STAT_TYPE;

#endif // ARION_LNX_KERNEL_UTILS_HPP
