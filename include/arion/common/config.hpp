#ifndef ARION_CONFIG_HPP
#define ARION_CONFIG_HPP

#include <any>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>

namespace arion
{

/// This class is responsible for storing a configuration which conditions the emulation of an Arion instance.
class ARION_EXPORT Config
{
  private:
    /// A map identifying a configuration value given its name. The value can be of any type for genericity purpose and
    /// must be casted.
    std::map<std::string, std::any> config_map = {
        {"log_lvl", LOG_LEVEL::INFO}, {"enable_sleep_syscalls", false}, {"thread_blocking_io", false}};

  public:
    /**
     * Defines the value of a field in the configuration. The value can be of any type for genericity purpose.
     * @tparam T Type of the value field.
     * @param key The name of the field to be defined.
     * @param value The new value for the field.
     */
    template <typename T> void ARION_EXPORT set_field(const std::string &key, T value)
    {
        auto it = this->config_map.find(key);
        if (it == this->config_map.end())
            throw arion_exception::ConfigKeyNotFoundException(key);
        it->second = value;
    }

    /**
     * Retrieves the value of a field from the configuration. The value can be of any type for genericity purpose.
     * @tparam T The type of the field to read.
     * @param key THe name of the field to retrieve.
     * @return The value of the field.
     */
    template <typename T> T ARION_EXPORT get_field(const std::string &key) const
    {
        auto it = this->config_map.find(key);
        if (it == this->config_map.end())
            throw arion_exception::ConfigKeyNotFoundException(key);
        try
        {
            return std::any_cast<T>(it->second);
        }
        catch (const std::bad_any_cast &)
        {
            throw arion_exception::ConfigWrongTypeAccessException(key);
        }
    }

    /**
     * Clones the configuration into a new one.
     * @return The new configuration.
     */
    Config ARION_EXPORT clone() const
    {
        Config newConfig;
        newConfig.config_map = this->config_map;
        return newConfig;
    }
};

}; // namespace arion

#endif // ARION_CONFIG_HPP
