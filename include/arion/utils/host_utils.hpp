#ifndef ARION_HOST_UTILS_HPP
#define ARION_HOST_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>

namespace arion
{

/**
 * Retrieves the CPU architecture of the host system.
 * This function uses preprocessor macros to determine the host architecture at compile time.
 * @return The `CPU_ARCH` enumeration value corresponding to the host system's architecture.
 * @throws arion_exception::UnsupportedHostCpuArchException If the host architecture is not supported
 * (though this is typically prevented by a compile-time error using `#error`).
 */
inline CPU_ARCH get_host_cpu_arch()
{
#if defined(__x86_64__) || defined(_M_X64)
    return CPU_ARCH::X8664_ARCH;
#elif defined(__i386__) || defined(_M_IX86)
    return CPU_ARCH::X86_ARCH;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return CPU_ARCH::ARM64_ARCH;
#elif defined(__arm__) || defined(_M_ARM)
    return CPU_ARCH::ARM_ARCH;
#else
#error "Unsupported host CPU architecture"
#endif
    // The throw statement is technically unreachable due to the #error above,
    // but kept for completeness if compilation proceeded without a known macro.
    throw arion_exception::UnsupportedHostCpuArchException();
}

}; // namespace arion

#endif // ARION_HOST_UTILS_HPP
