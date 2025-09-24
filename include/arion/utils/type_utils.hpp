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

};

namespace arion_type
{

class KernelType
{
  private:
    std::string name;
    arion::LOG_COLOR color;

  protected:
    KernelType(std::string name, arion::LOG_COLOR color = arion::LOG_COLOR::DEFAULT)
        : name(name), color(color) {};

  public:
    virtual ~KernelType() = default;
    arion::LOG_COLOR get_color();
    virtual std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val);
};

using InitFn = std::function<void()>;
class KernelTypeRegistry
{
  private:
    std::vector<std::tuple<int, InitFn>> initializers;
    bool initialized = false;

  public:
    static KernelTypeRegistry &instance()
    {
        static KernelTypeRegistry r;
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

#define ARION_REGISTER_KERNEL_TYPE(VAR, TYPE, PRIORITY)                                                                \
    static_assert(std::is_same_v<decltype(VAR), std::shared_ptr<TYPE>>,                                                \
                  "ARION_REGISTER_KERNEL_TYPE expects shared_ptr<TYPE>");                                              \
    static struct KernelTypeRegistrar_##VAR                                                                            \
    {                                                                                                                  \
        KernelTypeRegistrar_##VAR()                                                                                    \
        {                                                                                                              \
            KernelTypeRegistry::instance().register_type(PRIORITY, [] { VAR = std::make_shared<TYPE>(); });            \
        }                                                                                                              \
    } KernelTypeRegistrarInstance_##VAR;

class FlagType : public KernelType
{
  private:
    std::map<uint64_t, std::string> flag_map;

  protected:
    FlagType(std::string name, std::map<uint64_t, std::string> flag_map)
        : KernelType(name, arion::LOG_COLOR::CYAN), flag_map(flag_map) {};

  public:
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};

class IntType : public KernelType
{
  public:
    IntType() : KernelType("Int", arion::LOG_COLOR::MAGENTA) {};
};
extern std::shared_ptr<IntType> INT_TYPE;

class RawStringType : public KernelType
{
  public:
    RawStringType() : KernelType("Raw String", arion::LOG_COLOR::GREEN) {};
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<RawStringType> RAW_STRING_TYPE;

}; // namespace arion_type

#endif // ARION_TYPE_UTILS_HPP
