#ifndef ARION_CONFIG_HPP
#define ARION_CONFIG_HPP

#include <arion/common/global_defs.hpp>

struct ARION_EXPORT CONFIG
{
    arion::ARION_LOG_LEVEL log_lvl = arion::ARION_LOG_LEVEL::INFO;
    bool enable_sleep_syscall = false;
};

#endif // ARION_CONFIG_HPP
