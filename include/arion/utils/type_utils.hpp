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

/// Abstract base class for all kernel-related data types used for visualization/logging.
class KernelType
{
  private:
    /// Display name of the data type (e.g., "File descriptor").
    std::string name;
    /// The log color associated with this data type for display.
    arion::LOG_COLOR color;

  protected:
    /**
     * Builder for KernelType instances.
     * @param[in] name The display name of the type.
     * @param[in] color The associated log color. Default is `arion::LOG_COLOR::DEFAULT`.
     */
    KernelType(std::string name, arion::LOG_COLOR color = arion::LOG_COLOR::DEFAULT) : name(name), color(color) {};

  public:
    /// Virtual destructor to allow polymorphic deletion.
    virtual ~KernelType() = default;
    /**
     * Retrieves the color associated with this type.
     * @return The `arion::LOG_COLOR` enum value.
     */
    arion::LOG_COLOR get_color();
    /**
     * Converts a raw 64-bit value into a formatted string representation based on the type's semantics.
     * @param[in] arion Shared pointer to the main Arion instance (needed for memory access/context).
     * @param[in] val The raw 64-bit value to interpret.
     * @return The formatted string representation.
     */
    virtual std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val);
};

/// Function type used for initializing a kernel type.
using InitFn = std::function<void()>;
/// Singleton class responsible for managing the initialization order of all kernel type objects.
class KernelTypeRegistry
{
  private:
    /// List of initialization functions paired with their priority.
    std::vector<std::tuple<int, InitFn>> initializers;
    /// Flag indicating if initialization has been completed.
    bool initialized = false;

  public:
    /**
     * Provides access to the singleton instance of the registry.
     * @return Reference to the singleton instance.
     */
    static KernelTypeRegistry &instance()
    {
        static KernelTypeRegistry r;
        return r;
    }

    /**
     * Registers a type initialization function with a priority. Initialization runs in ascending priority order.
     * @param[in] priority The priority level for initialization.
     * @param[in] fn The function to call to initialize the type object.
     */
    void register_type(uint16_t priority, InitFn fn)
    {
        this->initializers.emplace_back(priority, fn);
    }

    /**
     * Executes all registered initialization functions in priority order.
     */
    void init_types()
    {
        this->initialized = true;
        std::sort(this->initializers.begin(), this->initializers.end(),
                  [](auto &a, auto &b) { return std::get<0>(a) < std::get<0>(b); });

        for (auto &[_, fn] : this->initializers)
            fn();
    }

    /**
     * Checks if the registry has finished initialization.
     * @return `true` if initialized, `false` otherwise.
     */
    bool is_initialized()
    {
        return this->initialized;
    }
};

/**
 * Macro to automatically register a shared pointer to a `KernelType` derived class.
 * This ensures that type objects are correctly instantiated in the required order via the `KernelTypeRegistry`.
 * @param VAR The `std::shared_ptr<TYPE>` variable to initialize.
 * @param TYPE The concrete class of the kernel type.
 * @param PRIORITY The initialization priority (lower numbers initialize first).
 */
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

/// Class for data types represented as a set of bit flags or discrete symbolic constants.
class FlagType : public KernelType
{
  private:
    /// Map linking raw integer flag values to their string names (e.g., `0x1` to "MAP_SHARED").
    std::map<uint64_t, std::string> flag_map;

  protected:
    /**
     * Builder for FlagType instances.
     * @param[in] name The display name of the type.
     * @param[in] flag_map The map of value-to-string mappings.
     */
    FlagType(std::string name, std::map<uint64_t, std::string> flag_map)
        : KernelType(name, arion::LOG_COLOR::CYAN), flag_map(flag_map) {};

  public:
    /**
     * Converts a raw value into a string representation, combining known flags (e.g., "FLAG_A|FLAG_B").
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] val The raw 64-bit flag value.
     * @return The formatted string representation of the flags.
     */
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};

/// Concrete type representing a generic integer value.
class IntType : public KernelType
{
  public:
    /**
     * Builder for IntType instances.
     */
    IntType() : KernelType("Int", arion::LOG_COLOR::MAGENTA) {};
};
/// Shared pointer to the singleton instance of `IntType`.
extern std::shared_ptr<IntType> INT_TYPE;

/// Concrete type representing a memory address pointing to a null-terminated string.
class RawStringType : public KernelType
{
  public:
    /**
     * Builder for RawStringType instances.
     */
    RawStringType() : KernelType("Raw String", arion::LOG_COLOR::GREEN) {};
    /**
     * Reads the null-terminated string starting at the memory address specified by `val`.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] val The memory address pointing to the string.
     * @return The content of the string read from memory.
     */
    std::string str(std::shared_ptr<arion::Arion> arion, uint64_t val) override;
};
/// Shared pointer to the singleton instance of `RawStringType`.
extern std::shared_ptr<RawStringType> RAW_STRING_TYPE;

}; // namespace arion_type

#endif // ARION_TYPE_UTILS_HPP
