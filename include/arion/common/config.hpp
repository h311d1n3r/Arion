#ifndef ARION_CONFIG_HPP
#define ARION_CONFIG_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/logger.hpp>

// General config can be set here
struct ARION_EXPORT CONFIG
{
    ARION_LOG_LEVEL log_lvl = ARION_LOG_LEVEL::INFO;
    bool enable_sleep_syscall = false;
};

#endif // ARION_CONFIG_HPP
