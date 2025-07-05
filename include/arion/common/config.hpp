#ifndef ARION_CONFIG_HPP
#define ARION_CONFIG_HPP

#include <any>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>

class ARION_EXPORT Config
{
  private:
    std::map<std::string, std::any> config_map = {
        {"log_lvl", arion::LOG_LEVEL::INFO}, {"enable_sleep_syscalls", false}, {"thread_blocking_io", false}};

  public:
    template <typename T> void ARION_EXPORT set_field(const std::string &key, T value)
    {
        auto it = this->config_map.find(key);
        if (it == this->config_map.end())
            throw ConfigKeyNotFoundException(key);
        it->second = value;
    }

    template <typename T> T ARION_EXPORT get_field(const std::string &key) const
    {
        auto it = this->config_map.find(key);
        if (it == this->config_map.end())
            throw ConfigKeyNotFoundException(key);
        try
        {
            return std::any_cast<T>(it->second);
        }
        catch (const std::bad_any_cast &)
        {
            throw ConfigWrongTypeAccessException(key);
        }
    }

    Config ARION_EXPORT clone() const
    {
        Config newConfig;
        newConfig.config_map = this->config_map;
        return newConfig;
    }
};

#endif // ARION_CONFIG_HPP
