#ifndef ARION_LOGGER_HPP
#define ARION_LOGGER_HPP

#include <arion/common/global_excepts.hpp>
#include <map>
#include <memory>
#include <sstream>
#ifdef ARION_ONLY
// spdlog should only be considered when compiling libarion.so as it is header-only
#include <arion/spdlog/spdlog.h>
#endif
#include <arion/common/global_defs.hpp>
#include <stack>

namespace arion
{

class Arion;

enum ARION_LOG_COLOR
{
    DEFAULT,
    BLACK,
    DARK_RED,
    DARK_GREEN,
    DARK_YELLOW,
    DARK_BLUE,
    DARK_MAGENTA,
    DARK_CYAN,
    LIGHT_GRAY,
    DARK_GRAY,
    RED,
    GREEN,
    ORANGE,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
};

inline std::map<ARION_LOG_COLOR, std::string> arion_log_colors_str = {
    {ARION_LOG_COLOR::DEFAULT, "\033[39m"},      {ARION_LOG_COLOR::BLACK, "\033[30m"},
    {ARION_LOG_COLOR::DARK_RED, "\033[31m"},     {ARION_LOG_COLOR::DARK_GREEN, "\033[32m"},
    {ARION_LOG_COLOR::DARK_YELLOW, "\033[33m"},  {ARION_LOG_COLOR::DARK_BLUE, "\033[34m"},
    {ARION_LOG_COLOR::DARK_MAGENTA, "\033[35m"}, {ARION_LOG_COLOR::DARK_CYAN, "\033[36m"},
    {ARION_LOG_COLOR::LIGHT_GRAY, "\033[37m"},   {ARION_LOG_COLOR::DARK_GRAY, "\033[90m"},
    {ARION_LOG_COLOR::RED, "\033[91m"},          {ARION_LOG_COLOR::GREEN, "\033[92m"},
    {ARION_LOG_COLOR::ORANGE, "\033[93m"},       {ARION_LOG_COLOR::BLUE, "\033[94m"},
    {ARION_LOG_COLOR::MAGENTA, "\033[95m"},      {ARION_LOG_COLOR::CYAN, "\033[96m"},
    {ARION_LOG_COLOR::WHITE, "\033[97m"},

};

class colorstream : public std::stringstream
{
  public:
    inline colorstream &operator<<(const ARION_LOG_COLOR &color)
    {
        (*static_cast<std::stringstream *>(this)) << arion_log_colors_str.at(color);
        return *this;
    }

    template <typename T> inline colorstream &operator<<(const T &value)
    {
        static_cast<std::stringstream &>(*this) << value;
        return *this;
    }

    using std::stringstream::operator<<;
};

class ARION_EXPORT Logger
{
  private:
    static uint64_t curr_id;
    static std::stack<uint64_t> free_logger_ids;
    static uint64_t gen_next_id();
    uint64_t id;
    pid_t curr_pid;
    pid_t curr_tid;
    LOG_LEVEL log_lvl;
    std::weak_ptr<Arion> arion;
#ifdef ARION_ONLY
    // spdlog should only be considered when compiling libarion.so as it is header-only
    std::shared_ptr<spdlog::logger> logger;
#else
    std::shared_ptr<void> logger;
#endif
    void refresh_prefix(bool force = false);

  public:
    static std::unique_ptr<Logger> initialize(std::weak_ptr<Arion> arion, LOG_LEVEL lvl = LOG_LEVEL::INFO);
    Logger(std::weak_ptr<Arion> arion);
    void ARION_EXPORT set_log_level(LOG_LEVEL lvl);
    LOG_LEVEL ARION_EXPORT get_log_level();
    void trace(std::string str);
    void debug(std::string str);
    void info(std::string str);
    void warn(std::string str);
    void error(std::string str);
    void critical(std::string str);
};

}; // namespace arion

#endif // ARION_LOGGER_HPP
