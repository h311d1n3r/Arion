#ifndef ARION_TYPE_UTILS_HPP
#define ARION_TYPE_UTILS_HPP

#include <arion/common/logger.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define COMMON_TYPE_PRIORITY 0
#define OS_BASE_TYPE_PRIORITY 1
#define OS_STRUCT_FACTORY_PRIORITY 2
#define OS_STRUCT_TYPE_PRIORITY 3
#define OS_VARIABLE_STRUCT_TYPE_PRIORITY 4

namespace arion
{

class Arion;

class ArionType
{
  private:
    std::string name;
    ARION_LOG_COLOR color;

  protected:
    ArionType(std::string name, ARION_LOG_COLOR color = ARION_LOG_COLOR::DEFAULT) : name(name), color(color) {};

  public:
    virtual ~ArionType() = default;
    ARION_LOG_COLOR get_color();
    virtual std::string str(std::shared_ptr<Arion> arion, uint64_t val);
};

using InitFn = std::function<void()>;
class ArionTypeRegistry
{
  private:
    std::vector<std::tuple<int, InitFn>> initializers;
    bool initialized = false;

  public:
    static ArionTypeRegistry &instance()
    {
        static ArionTypeRegistry r;
        return r;
    }

    void register_type(uint16_t priority, InitFn fn)
    {
        this->initializers.emplace_back(priority, fn);
    }

    void init_types()
    {
        this->initialized = true;
        std::sort(this->initializers.begin(), this->initializers.end(),
                  [](auto &a, auto &b) { return std::get<0>(a) < std::get<0>(b); });

        for (auto &[_, fn] : this->initializers)
            fn();
    }

    bool is_initialized()
    {
        return this->initialized;
    }
};

#define REGISTER_ARION_TYPE(VAR, TYPE, PRIORITY)                                                                       \
    static_assert(std::is_same_v<decltype(VAR), std::shared_ptr<TYPE>>,                                                \
                  "REGISTER_ARION_TYPE expects shared_ptr<TYPE>");                                                     \
    static struct ArionTypeRegistrar_##VAR                                                                             \
    {                                                                                                                  \
        ArionTypeRegistrar_##VAR()                                                                                     \
        {                                                                                                              \
            ArionTypeRegistry::instance().register_type(PRIORITY, [] { VAR = std::make_shared<TYPE>(); });             \
        }                                                                                                              \
    } ArionTypeRegistrarInstance_##VAR;

class ArionFlagType : public ArionType
{
  private:
    std::map<uint64_t, std::string> flag_map;

  protected:
    ArionFlagType(std::string name, std::map<uint64_t, std::string> flag_map)
        : ArionType(name, ARION_LOG_COLOR::CYAN), flag_map(flag_map) {};

  public:
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};

class ArionIntType : public ArionType
{
  public:
    ArionIntType() : ArionType("Int", ARION_LOG_COLOR::MAGENTA) {};
};
extern std::shared_ptr<ArionIntType> ARION_INT_TYPE;

class ArionRawStringType : public ArionType
{
  public:
    ArionRawStringType() : ArionType("Raw String", ARION_LOG_COLOR::GREEN) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionRawStringType> ARION_RAW_STRING_TYPE;

}; // namespace arion

#endif // ARION_TYPE_UTILS_HPP
