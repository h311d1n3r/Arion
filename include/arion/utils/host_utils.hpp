#ifndef ARION_HOST_UTILS_HPP
#define ARION_HOST_UTILS_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>

inline arion::CPU_ARCH get_host_cpu_arch()
{
#if defined(__x86_64__) || defined(_M_X64)
    return arion::CPU_ARCH::X8664_ARCH;
#elif defined(__i386__) || defined(_M_IX86)
    return arion::CPU_ARCH::X86_ARCH;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return arion::CPU_ARCH::ARM64_ARCH;
#elif defined(__arm__) || defined(_M_ARM)
    return arion::CPU_ARCH::ARM_ARCH;
#else
#error "Unsupported host CPU architecture"
#endif
    throw UnsupportedHostCpuArchException();
}

#endif // ARION_HOST_UTILS_HPP
