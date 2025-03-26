#ifndef ARION_LOGGER_HPP
#define ARION_LOGGER_HPP

#include <arion/common/global_excepts.hpp>
#include <map>
#include <memory>
#include <arion/spdlog/spdlog.h>
#include <arion/common/global_defs.hpp>
#include <stack>

class Arion;

inline std::map<arion::ARION_LOG_LEVEL, spdlog::level::level_enum> arion_log_lvl_to_spdlog = {
    {arion::ARION_LOG_LEVEL::TRACE, spdlog::level::level_enum::trace},
    {arion::ARION_LOG_LEVEL::DEBUG, spdlog::level::level_enum::debug},
    {arion::ARION_LOG_LEVEL::INFO, spdlog::level::level_enum::info},
    {arion::ARION_LOG_LEVEL::WARN, spdlog::level::level_enum::warn},
    {arion::ARION_LOG_LEVEL::ERROR, spdlog::level::level_enum::err},
    {arion::ARION_LOG_LEVEL::CRITICAL, spdlog::level::level_enum::critical},
    {arion::ARION_LOG_LEVEL::OFF, spdlog::level::level_enum::off},
};

class ARION_EXPORT Logger
{
  private:
    static uint64_t curr_id;
    static std::stack<uint64_t> free_logger_ids;
    static uint64_t gen_next_id();
    uint64_t id;
    arion::ARION_LOG_LEVEL log_lvl;
    std::weak_ptr<Arion> arion;
    std::shared_ptr<spdlog::logger> logger;

  public:
    static std::unique_ptr<Logger> initialize(std::weak_ptr<Arion> arion, arion::ARION_LOG_LEVEL lvl = arion::ARION_LOG_LEVEL::INFO);
    Logger(std::weak_ptr<Arion> arion);
    void ARION_EXPORT set_log_level(arion::ARION_LOG_LEVEL lvl);
    arion::ARION_LOG_LEVEL ARION_EXPORT get_log_level();
    void trace(std::string str);
    void debug(std::string str);
    void info(std::string str);
    void warn(std::string str);
    void error(std::string str);
    void critical(std::string str);
};

#endif // ARION_LOGGER_HPP
