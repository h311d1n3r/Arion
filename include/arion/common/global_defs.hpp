#ifndef ARION_GLOBAL_DEFS_HPP
#define ARION_GLOBAL_DEFS_HPP

#include <array>
#include <string>
#include <map>
#include <cstdint>
#include <cstring>
#include <stddef.h>
#include <sys/types.h>

#define ARION_EXPORT __attribute__((visibility("default")))
#define ARION_BUF_SZ 0x1000
#define ARION_UUID_SZ 0x24
#define ARION_SYSTEM_PAGE_SZ 0x1000
#define ARION_FD_SZ 0x4
#define ARION_UNIX_PATH_MAX 0x6C
#define ARION_MAX_U32 0xFFFFFFFF
#define ARION_MAX_U64 0xFFFFFFFFFFFFFFFF
#define ARION_PROCESS_PID 0x1
#define ARION_CYCLES_PER_THREAD 0x1000

namespace arion
{
using ADDR = uint64_t;
using PROT_FLAGS = uint8_t;
using BYTE = uint8_t;
using REG = uint64_t;
using RVAL8 = uint8_t;
using RVAL16 = uint16_t;
using RVAL32 = uint32_t;
using RVAL64 = uint64_t;
using RVAL128 = std::array<BYTE, 16>;
using RVAL256 = std::array<BYTE, 32>;
using RVAL512 = std::array<BYTE, 64>;
using SYS_PARAM = uint64_t;

const size_t EMPTY = 0;

union ARION_EXPORT RVAL {
    RVAL8 r8;
    RVAL16 r16;
    RVAL32 r32;
    RVAL64 r64;
    RVAL128 r128;
    RVAL256 r256;
    RVAL512 r512;
};

struct SEGMENT
{
    ADDR virt_addr;
    ADDR file_addr;
    size_t align;
    size_t virt_sz;
    size_t phy_sz;
    PROT_FLAGS flags;
};

struct SIGNAL
{
    pid_t source_pid;
    int signo;
    SIGNAL(pid_t source_pid, int signo) : source_pid(source_pid), signo(signo) {};
};

enum LINKAGE_TYPE
{
    UNKNOWN_LINKAGE,
    DYNAMIC_LINKAGE,
    STATIC_LINKAGE
};

enum ARION_EXPORT CPU_ARCH
{
    UNKNOWN_ARCH,
    X86_ARCH,
    X8664_ARCH,
    ARM_ARCH,
    ARM64_ARCH
};

enum ARION_EXPORT ARION_LOG_LEVEL
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
    OFF
};

inline std::map<std::string, CPU_ARCH> ARCH_FROM_NAME
{
    {"UNKNOWN", CPU_ARCH::UNKNOWN_ARCH},
    {"X86", CPU_ARCH::X86_ARCH},
    {"X86-64", CPU_ARCH::X8664_ARCH},
    {"ARM", CPU_ARCH::ARM_ARCH},
    {"ARM64", CPU_ARCH::ARM64_ARCH}
};

inline std::map<CPU_ARCH, std::string> NAME_FROM_ARCH
{
    {CPU_ARCH::UNKNOWN_ARCH, "UNKNOWN"},
    {CPU_ARCH::X86_ARCH, "X86"},
    {CPU_ARCH::X8664_ARCH, "X86-64"},
    {CPU_ARCH::ARM_ARCH, "ARM"},
    {CPU_ARCH::ARM64_ARCH, "ARM64"}
};

} // namespace arion

#endif // ARION_GLOBAL_DEFS_HPP
