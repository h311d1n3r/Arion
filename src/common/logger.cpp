#include <arion/arion.hpp>
#include <arion/common/logger.hpp>
#include <arion/spdlog/sinks/stdout_color_sinks.h>
#include <memory>

using namespace arion;

inline std::map<arion::ARION_LOG_LEVEL, spdlog::level::level_enum> ARION_LOG_LVL_TO_SPDLOG = {
    {arion::ARION_LOG_LEVEL::TRACE, spdlog::level::level_enum::trace},
    {arion::ARION_LOG_LEVEL::DEBUG, spdlog::level::level_enum::debug},
    {arion::ARION_LOG_LEVEL::INFO, spdlog::level::level_enum::info},
    {arion::ARION_LOG_LEVEL::WARN, spdlog::level::level_enum::warn},
    {arion::ARION_LOG_LEVEL::ERROR, spdlog::level::level_enum::err},
    {arion::ARION_LOG_LEVEL::CRITICAL, spdlog::level::level_enum::critical},
    {arion::ARION_LOG_LEVEL::OFF, spdlog::level::level_enum::off},
};

uint64_t Logger::curr_id = 1;

std::stack<uint64_t> Logger::free_logger_ids;

uint64_t Logger::gen_next_id()
{
    uint64_t logger_id;
    if (!Logger::free_logger_ids.empty())
    {
        logger_id = Logger::free_logger_ids.top();
        Logger::free_logger_ids.pop();
    }
    else
    {
        if (Logger::curr_id == ARION_MAX_U64)
            throw TooManyLoggersException();
        logger_id = Logger::curr_id++;
    }
    return logger_id;
}

std::unique_ptr<Logger> Logger::initialize(std::weak_ptr<Arion> arion, ARION_LOG_LEVEL lvl)
{
    std::unique_ptr<Logger> logger = std::make_unique<Logger>(arion);
    logger->set_log_level(lvl);
    return std::move(logger);
}

Logger::Logger(std::weak_ptr<Arion> arion) : arion(arion)
{
    this->id = Logger::gen_next_id();
    std::shared_ptr<Arion> arion_ = this->arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");
    this->logger = spdlog::stdout_color_mt(int_to_hex<uint64_t>(this->id) + std::string(" - PID ") +
                                           int_to_hex<pid_t>(arion_->get_pid()));
}

void Logger::set_log_level(ARION_LOG_LEVEL lvl)
{
    auto lvl_it = ARION_LOG_LVL_TO_SPDLOG.find(lvl);
    if (lvl_it == ARION_LOG_LVL_TO_SPDLOG.end())
        throw WrongLogLevelException();
    this->logger->set_level(lvl_it->second);
    this->log_lvl = lvl;
}

ARION_LOG_LEVEL Logger::get_log_level()
{
    return this->log_lvl;
}

void Logger::trace(std::string str)
{
    this->logger->trace(str);
}

void Logger::debug(std::string str)
{
    this->logger->debug(str);
}

void Logger::info(std::string str)
{
    this->logger->info(str);
}

void Logger::warn(std::string str)
{
    this->logger->warn(str);
}

void Logger::error(std::string str)
{
    this->logger->error(str);
}

void Logger::critical(std::string str)
{
    this->logger->critical(str);
}
