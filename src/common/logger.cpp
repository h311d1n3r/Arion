#include <arion/arion.hpp>
#include <arion/common/logger.hpp>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace arion;

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
    auto lvl_it = arion_log_lvl_to_spdlog.find(lvl);
    if (lvl_it == arion_log_lvl_to_spdlog.end())
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
