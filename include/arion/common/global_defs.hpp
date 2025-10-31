#ifndef ARION_GLOBAL_DEFS_HPP
#define ARION_GLOBAL_DEFS_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <stddef.h>
#include <string>
#include <sys/types.h>

/// Defines which symbols should be exported from the library.
#define ARION_EXPORT __attribute__((visibility("default")))
/// Defines a generic value that many buffers use to initialize their size.
#define ARION_BUF_SZ 0x1000
/// Size of a UUID buffer.
#define ARION_UUID_SZ 0x24
/// The 4 KB memory page size that most systems use.
#define ARION_SYSTEM_PAGE_SZ 0x1000
/// Size of a file descriptor (int).
#define ARION_FD_SZ 0x4
/// Maximum size for a UNIX path.
#define ARION_UNIX_PATH_MAX 0x6C
/// Maximum number for an unsigned 32-bit integer.
#define ARION_MAX_U32 0xFFFFFFFF
/// Maximum number for an unsigned 64-bit integer.
#define ARION_MAX_U64 0xFFFFFFFFFFFFFFFF
/// First PID value to be associated with an emulated process.
#define ARION_PROCESS_PID 0x1
/// Number of CPU cycles for a thread before switching to another one.
#define ARION_CYCLES_PER_THREAD 0x1000

namespace arion
{
/// Identifies a memory address.
using ADDR = uint64_t;
/// Identifies memory protection rights.
using PROT_FLAGS = uint8_t;
/// Identifies an 8-bit integer.
using BYTE = uint8_t;
/// Identifies a Unicorn register.
using REG = uint64_t;
/// Identifies a 8-bit register value.
using RVAL8 = uint8_t;
/// Identifies a 16-bit register value.
using RVAL16 = uint16_t;
/// Identifies a 32-bit register value.
using RVAL32 = uint32_t;
/// Identifies a 64-bit register value.
using RVAL64 = uint64_t;
/// Identifies a 128-bit register value.
using RVAL128 = std::array<BYTE, 16>;
/// Identifies a 256-bit register value.
using RVAL256 = std::array<BYTE, 32>;
/// Identifies a 512-bit register value.
using RVAL512 = std::array<BYTE, 64>;
/// Identifies a syscall parameter.
using SYS_PARAM = uint64_t;

/// A 64-bit zero-initialized value for various purposes.
const size_t EMPTY = 0;

/// Identifies a size-agnostic register value.
union ARION_EXPORT RVAL {
    /// Used to store 8-bit register values.
    RVAL8 r8;
    /// Used to store 16-bit register values.
    RVAL16 r16;
    /// Used to store 32-bit register values.
    RVAL32 r32;
    /// Used to store 64-bit register values.
    RVAL64 r64;
    /// Used to store 128-bit register values.
    RVAL128 r128;
    /// Used to store 256-bit register values.
    RVAL256 r256;
    /// Used to store 512-bit register values.
    RVAL512 r512;
};

/// Identifies an OS memory segment.
struct SEGMENT
{
    /// A string associated to this memory segment, for identification purpose.
    std::string info;
    /// The address where this segment should stand in memory.
    ADDR virt_addr;
    /// The offset where data for this segment holds in its associated file.
    ADDR file_addr;
    /// The byte alignment value that this segment must respect when being mapped in memory.
    size_t align;
    /// The size of this segment when allocated in memory.
    size_t virt_sz;
    /// The size of this segment in the file that describes it.
    size_t phy_sz;
    /// Memory protection rights for this segment.
    PROT_FLAGS flags;
};

/// Identifies an OS signal.
struct SIGNAL
{
    /// PID of the process responsible for triggering this signal.
    pid_t source_pid;
    /// The signal number.
    int signo;
    /**
     * Builder for SIGNAL instances.
     * @param[in] source_pid PID of the process responsible for triggering this signal.
     * @param[in] signo The signal number.
     */
    SIGNAL(pid_t source_pid, int signo) : source_pid(source_pid), signo(signo) {};
};

/// Identifies the method used to link an executable.
enum LINKAGE_TYPE
{
    /// Unknown linkage method.
    UNKNOWN_LINKAGE,
    /// The executable was dynamically linked.
    DYNAMIC_LINKAGE,
    /// The executable was statically linked.
    STATIC_LINKAGE
};

/// Identifies a CPU architecture.
enum ARION_EXPORT CPU_ARCH
{
    /// Unknown CPU architecture.
    UNKNOWN_ARCH,
    /// The x86 CPU architecture.
    X86_ARCH,
    /// The x86-64 CPU architecture.
    X8664_ARCH,
    /// The ARM CPU architecture.
    ARM_ARCH,
    /// The ARM64 CPU architecture.
    ARM64_ARCH,
    /// The PowerPC 32-bit CPU architecture.
    PPC32_ARCH
};

/// Identifies an Operating System (OS).
enum ARION_EXPORT PLATFORM
{
    /// Unknown OS.
    UNKNOWN_PLATFORM,
    /// Linux.
    LINUX,
    /// Windows.
    WINDOWS,
    /// macOS.
    MACOS
};

/// Identifies a logging level.
enum ARION_EXPORT LOG_LEVEL
{
    /// Fine-grained informational events, typically for debugging.
    TRACE,
    /// Debug-level messages, useful for development and troubleshooting.
    DEBUG,
    /// General informational messages about application progress.
    INFO,
    /// Potentially harmful situations or warnings.
    WARN,
    /// Error events that might allow the application to continue running.
    ERROR,
    /// Severe error events that will likely lead to application termination.
    CRITICAL,
    /// Disables all logging output.
    OFF
};

/// A map identifying a CPU architecture given its name.
inline std::map<std::string, CPU_ARCH> ARCH_FROM_NAME{{"UNKNOWN", CPU_ARCH::UNKNOWN_ARCH},
                                                      {"X86", CPU_ARCH::X86_ARCH},
                                                      {"X86-64", CPU_ARCH::X8664_ARCH},
                                                      {"ARM", CPU_ARCH::ARM_ARCH},
                                                      {"ARM64", CPU_ARCH::ARM64_ARCH},
                                                      {"PPC32", CPU_ARCH::PPC32_ARCH}};

/// A map identifying a name corresponding a given CPU architecture.
inline std::map<CPU_ARCH, std::string> NAME_FROM_ARCH{{CPU_ARCH::UNKNOWN_ARCH, "UNKNOWN"},
                                                      {CPU_ARCH::X86_ARCH, "X86"},
                                                      {CPU_ARCH::X8664_ARCH, "X86-64"},
                                                      {CPU_ARCH::ARM_ARCH, "ARM"},
                                                      {CPU_ARCH::ARM64_ARCH, "ARM64"},
                                                      {CPU_ARCH::PPC32_ARCH, "PPC32"}};

} // namespace arion

#endif // ARION_GLOBAL_DEFS_HPP
