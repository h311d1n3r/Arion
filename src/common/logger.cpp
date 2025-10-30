#include <arion/arion.hpp>
#include <arion/common/logger.hpp>
#include <arion/spdlog/sinks/stdout_color_sinks.h>
#include <memory>

using namespace arion;
using namespace arion_exception;

inline std::map<arion::LOG_LEVEL, spdlog::level::level_enum> ARION_LOG_LVL_TO_SPDLOG = {
    {arion::LOG_LEVEL::TRACE, spdlog::level::level_enum::trace},
    {arion::LOG_LEVEL::DEBUG, spdlog::level::level_enum::debug},
    {arion::LOG_LEVEL::INFO, spdlog::level::level_enum::info},
    {arion::LOG_LEVEL::WARN, spdlog::level::level_enum::warn},
    {arion::LOG_LEVEL::ERROR, spdlog::level::level_enum::err},
    {arion::LOG_LEVEL::CRITICAL, spdlog::level::level_enum::critical},
    {arion::LOG_LEVEL::OFF, spdlog::level::level_enum::off}};

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

std::unique_ptr<Logger> Logger::initialize(std::weak_ptr<Arion> arion, LOG_LEVEL lvl)
{
    std::unique_ptr<Logger> logger = std::make_unique<Logger>(arion);
    logger->curr_pid = 0;
    logger->curr_tid = 0;
    logger->set_log_level(lvl);
    return std::move(logger);
}

Logger::Logger(std::weak_ptr<Arion> arion) : arion(arion)
{
    this->id = Logger::gen_next_id();
    std::shared_ptr<Arion> arion_ = this->arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");
    this->logger = spdlog::stdout_color_mt(int_to_hex<uint64_t>(this->id));
    this->refresh_prefix(true);
}

void Logger::set_log_level(LOG_LEVEL lvl)
{
    auto lvl_it = ARION_LOG_LVL_TO_SPDLOG.find(lvl);
    if (lvl_it == ARION_LOG_LVL_TO_SPDLOG.end())
        throw WrongLogLevelException();
    this->logger->set_level(lvl_it->second);
    this->log_lvl = lvl;
}

LOG_LEVEL Logger::get_log_level()
{
    return this->log_lvl;
}

void Logger::refresh_prefix(bool force)
{
    pid_t pid = 0;
    pid_t tid = 0;
    std::shared_ptr<Arion> arion_ = this->arion.lock();
    if (arion_ && arion_->threads)
    {
        pid = arion_->get_pid();
        tid = arion_->threads->get_running_tid();
    }
    if (force || pid != this->curr_pid || tid != this->curr_tid)
    {
        this->curr_pid = pid;
        this->curr_tid = tid;
        colorstream cs;
        cs << LOG_COLOR::WHITE << "[" << LOG_COLOR::ORANGE << "%Y-%m-%d %H:%M:%S.%e"
           << LOG_COLOR::WHITE << "] [" << LOG_COLOR::GREEN << int_to_hex<uint64_t>(this->id)
           << LOG_COLOR::WHITE << ", ";
        if (pid && tid)
            cs << LOG_COLOR::RED << "PID=" << int_to_hex<pid_t>(pid) << LOG_COLOR::WHITE << ", "
               << LOG_COLOR::MAGENTA << "TID=" << int_to_hex<pid_t>(tid);
        else
            cs << LOG_COLOR::RED << "NOT RUNNING";
        cs << LOG_COLOR::WHITE << "] [%^%l%$] %v";
        this->logger->set_pattern(cs.str());
    }
}

void Logger::trace(std::string str)
{
    this->refresh_prefix();
    this->logger->trace(str + arion_log_colors_str.at(LOG_COLOR::DEFAULT));
}

void Logger::debug(std::string str)
{
    this->refresh_prefix();
    this->logger->debug(str + arion_log_colors_str.at(LOG_COLOR::DEFAULT));
}

void Logger::info(std::string str)
{
    this->refresh_prefix();
    this->logger->info(str + arion_log_colors_str.at(LOG_COLOR::DEFAULT));
}

void Logger::warn(std::string str)
{
    this->refresh_prefix();
    this->logger->warn(str + arion_log_colors_str.at(LOG_COLOR::DEFAULT));
}

void Logger::error(std::string str)
{
    this->refresh_prefix();
    this->logger->error(str + arion_log_colors_str.at(LOG_COLOR::DEFAULT));
}

void Logger::critical(std::string str)
{
    this->refresh_prefix();
    this->logger->critical(str + arion_log_colors_str.at(LOG_COLOR::DEFAULT));
}
