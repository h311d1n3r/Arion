#ifndef ARION_LOGGER_HPP
#define ARION_LOGGER_HPP

#include <arion/common/global_excepts.hpp>
#include <map>
#include <memory>
#ifdef ARION_ONLY
// spdlog should only be considered when compiling libarion.so as it is header-only
#include <arion/spdlog/spdlog.h>
#endif
#include <arion/common/global_defs.hpp>
#include <stack>

class Arion;

class ARION_EXPORT Logger
{
  private:
    static uint64_t curr_id;
    static std::stack<uint64_t> free_logger_ids;
    static uint64_t gen_next_id();
    uint64_t id;
    arion::ARION_LOG_LEVEL log_lvl;
    std::weak_ptr<Arion> arion;
#ifdef ARION_ONLY
    // spdlog should only be considered when compiling libarion.so as it is header-only
    std::shared_ptr<spdlog::logger> logger;
#else
    std::shared_ptr<void> logger;
#endif

  public:
    static std::unique_ptr<Logger> initialize(std::weak_ptr<Arion> arion,
                                              arion::ARION_LOG_LEVEL lvl = arion::ARION_LOG_LEVEL::INFO);
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
