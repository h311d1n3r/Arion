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

/// These colors can be used for logging.
enum LOG_COLOR
{
    DEFAULT,      ///< Default foreground color.
    BLACK,        ///< Black.
    DARK_RED,     ///< Dark red.
    DARK_GREEN,   ///< Dark green.
    DARK_YELLOW,  ///< Dark yellow.
    DARK_BLUE,    ///< Dark blue.
    DARK_MAGENTA, ///< Dark magenta.
    DARK_CYAN,    ///< Dark cyan.
    LIGHT_GRAY,   ///< Light gray.
    DARK_GRAY,    ///< Dark gray.
    RED,          ///< Red.
    GREEN,        ///< Green.
    ORANGE,       ///< Orange.
    BLUE,         ///< Blue.
    MAGENTA,      ///< Magenta.
    CYAN,         ///< Cyan.
    WHITE         ///< White.
};

/// A map identifying an ANSI color code given its associated Arion LOG_COLOR.
inline std::map<LOG_COLOR, std::string> arion_log_colors_str = {
    {LOG_COLOR::DEFAULT, "\033[39m"},      {LOG_COLOR::BLACK, "\033[30m"},       {LOG_COLOR::DARK_RED, "\033[31m"},
    {LOG_COLOR::DARK_GREEN, "\033[32m"},   {LOG_COLOR::DARK_YELLOW, "\033[33m"}, {LOG_COLOR::DARK_BLUE, "\033[34m"},
    {LOG_COLOR::DARK_MAGENTA, "\033[35m"}, {LOG_COLOR::DARK_CYAN, "\033[36m"},   {LOG_COLOR::LIGHT_GRAY, "\033[37m"},
    {LOG_COLOR::DARK_GRAY, "\033[90m"},    {LOG_COLOR::RED, "\033[91m"},         {LOG_COLOR::GREEN, "\033[92m"},
    {LOG_COLOR::ORANGE, "\033[93m"},       {LOG_COLOR::BLUE, "\033[94m"},        {LOG_COLOR::MAGENTA, "\033[95m"},
    {LOG_COLOR::CYAN, "\033[96m"},         {LOG_COLOR::WHITE, "\033[97m"},

};

/// A stringstream extended class, which allows the use of LOG_COLOR colors for logging.
class colorstream : public std::stringstream
{
  public:
    /**
     * The redefined stream insertion operator for coloring purpose.
     * @param[in] color Color of the next text to be inserted in the current colorstream.
     * @return The current colorstream instance.
     */
    inline colorstream &operator<<(const LOG_COLOR &color)
    {
        (*static_cast<std::stringstream *>(this)) << arion_log_colors_str.at(color);
        return *this;
    }

    /**
     * Stream insertion operator for generic types.
     * Allows inserting any type that supports the standard stream insertion operator.
     * @tparam T Type of the value being inserted into the stream.
     * @param[in] value The value to insert into the stream.
     * @return The current colorstream instance.
     */
    template <typename T> inline colorstream &operator<<(const T &value)
    {
        static_cast<std::stringstream &>(*this) << value;
        return *this;
    }

    /// Inherit all standard stream insertion operators from std::stringstream.
    using std::stringstream::operator<<;
};

/// This class is responsible for managing logging in the Arion emulation framework.
class ARION_EXPORT Logger
{
  private:
    /// The unique identifier for this logger instance.
    static uint64_t curr_id;
    /// A stack of free logger IDs that can be reused.
    static std::stack<uint64_t> free_logger_ids;
    /// Generates the next free logger ID.
    static uint64_t gen_next_id();
    /// The unique identifier for this logger instance.
    uint64_t id;
    /// The process ID of the current process.
    pid_t curr_pid;
    /// The thread ID of the current thread.
    pid_t curr_tid;
    /// The current log level.
    LOG_LEVEL log_lvl;
    /// The Arion instance associated to this instance.
    std::weak_ptr<Arion> arion;
#ifdef ARION_ONLY
    // spdlog should only be considered when compiling libarion.so as it is header-only
    /// The spdlog logger instance.
    std::shared_ptr<spdlog::logger> logger;
#else
    /// The spdlog logger instance.
    std::shared_ptr<void> logger;
#endif
    /**
     * Refreshes the log prefix if needed.
     * @param[in] force True if the prefix should be reloaded in all cases.
     */
    void refresh_prefix(bool force = false);

  public:
    /**
     * Instanciates and initializes new Logger objects with some parameters.
     * @param[in] arion The Arion instance associated with this instance.
     * @param[in] lvl The initial log level.
     * @return A new Logger instance.
     */
    static std::unique_ptr<Logger> initialize(std::weak_ptr<Arion> arion, LOG_LEVEL lvl = LOG_LEVEL::INFO);
    /**
     * Builder for Logger instances.
     * @param[in] arion The Arion instance associated with this instance.
     */
    Logger(std::weak_ptr<Arion> arion);
    /**
     * Defines the log level.
     * @param[in] lvl The new log level.
     */
    void ARION_EXPORT set_log_level(LOG_LEVEL lvl);
    /**
     * Retrieves the current log level.
     * @return The current log level.
     */
    LOG_LEVEL ARION_EXPORT get_log_level();
    /**
     * Logs a trace message.
     * @param[in] str The message to log.
     */
    void trace(std::string str);
    /**
     * Logs a debug message.
     * @param[in] str The message to log.
     */
    void debug(std::string str);
    /**
     * Logs an info message.
     * @param[in] str The message to log.
     */
    void info(std::string str);
    /**
     * Logs a warning message.
     * @param[in] str The message to log.
     */
    void warn(std::string str);
    /**
     * Logs an error message.
     * @param[in] str The message to log.
     */
    void error(std::string str);
    /**
     * Logs a critical message.
     * @param[in] str The message to log.
     */
    void critical(std::string str);
};

}; // namespace arion

#endif // ARION_LOGGER_HPP
