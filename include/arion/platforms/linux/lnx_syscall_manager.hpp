#ifndef ARION_LNX_SYSCALL_MANAGER_HPP
#define ARION_LNX_SYSCALL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/utils/type_utils.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace arion
{

class Arion;

/// Type alias for system call parameters.
using SYS_PARAM = uint64_t;

/// Structure defining a system call function and its parameter signature for logging/analysis.
struct SYSCALL_FUNC
{
    /// The function pointer to the syscall emulation handler.
    std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)> func;
    /// A vector defining the expected data type for each syscall parameter and return value, used for
    /// logging/formatting.
    std::vector<std::shared_ptr<arion_type::KernelType>> signature;

    /**
     * Builder for SYSCALL_FUNC instances.
     * @param[in] func The system call emulation function handler.
     * @param[in] signature The type signature of the syscall (including the return type as the first element).
     */
    SYSCALL_FUNC(
        std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)> func,
        std::vector<std::shared_ptr<arion_type::KernelType>> signature)
        : func(func), signature(signature) {};
};

/// Global map storing the number of parameters required for a syscall, indexed by its string name.
extern std::map<std::string, uint8_t> PARAMS_N_BY_SYSCALL_NAME;

/// Manages the emulation and dispatching of Linux system calls.
class ARION_EXPORT LinuxSyscallManager
{
  private:
    /// Weak pointer to the main Arion instance.
    std::weak_ptr<Arion> arion;
    /// Map linking the numeric syscall number (`sysno`) to its emulation function and signature.
    std::map<uint64_t, std::shared_ptr<SYSCALL_FUNC>> syscall_funcs;
    /**
     * Adds an entry to the internal `syscall_funcs` map, looking up the numeric syscall number by its name.
     * @param[in] name The string name of the syscall (e.g., "sys_read").
     * @param[in] func Shared pointer to the syscall function and signature structure.
     */
    void add_syscall_entry(std::string name, std::shared_ptr<SYSCALL_FUNC> func);
    /**
     * Template helper function to easily create a shared pointer to a `SYSCALL_FUNC` instance, converting a variadic
     * list of `KernelType` arguments into the signature vector.
     * @tparam SignatureArgs Types of the kernel types making up the signature.
     * @param[in] func The system call emulation function.
     * @param[in] signature The `KernelType` objects defining the return type and parameter types.
     * @return A shared pointer to the new `SYSCALL_FUNC` structure.
     */
    template <typename... SignatureArgs>
    std::shared_ptr<SYSCALL_FUNC> make_sys_func(
        std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)> func,
        SignatureArgs &&...signature)
    {
        std::vector<std::shared_ptr<arion_type::KernelType>> sig_vec = {std::forward<SignatureArgs>(signature)...};
        return std::make_shared<SYSCALL_FUNC>(func, sig_vec);
    }
    /**
     * Populates the `syscall_funcs` map by initializing all known syscall emulation functions.
     */
    void init_syscall_funcs();
    /**
     * Logs the executed syscall, formatting parameters and return value based on the syscall's signature.
     * @param[in] arion Shared pointer to the Arion instance.
     * @param[in] sys_name The string name of the syscall.
     * @param[in] signature The type signature of the syscall.
     * @param[in] func_params The actual raw parameter values passed to the syscall.
     * @param[in] syscall_ret The raw return value of the syscall.
     */
    void print_syscall(std::shared_ptr<Arion> arion, std::string sys_name,
                       std::vector<std::shared_ptr<arion_type::KernelType>> signature,
                       std::vector<SYS_PARAM> func_params, uint64_t syscall_ret);

  public:
    /**
     * Static method to create and initialize a new `LinuxSyscallManager` instance.
     * @param[in] arion Weak pointer to the main Arion instance.
     * @return A unique pointer to the initialized manager.
     */
    static std::unique_ptr<LinuxSyscallManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for LinuxSyscallManager instances.
     * @param[in] arion Weak pointer to the main Arion instance.
     */
    LinuxSyscallManager(std::weak_ptr<Arion> arion);
    /**
     * Main entry point for syscall handling: reads registers, looks up the function, executes the handler, and writes
     * the return value. Also handles logic for syscall hooks and canceling/deferring execution based on the `cancel`
     * boolean.
     * @param[in] arion Shared pointer to the Arion instance.
     */
    void process_syscall(std::shared_ptr<Arion> arion);
    /**
     * Sets a custom syscall emulation function by its numeric syscall number.
     * @param[in] sysno The numeric syscall number.
     * @param[in] func Shared pointer to the new syscall function structure.
     */
    void ARION_EXPORT set_syscall_func(uint64_t sysno, std::shared_ptr<SYSCALL_FUNC> func);
    /**
     * Sets a custom syscall emulation function by its string name.
     * @param[in] name The string name of the syscall.
     * @param[in] func Shared pointer to the new syscall function structure.
     */
    void ARION_EXPORT set_syscall_func(std::string name, std::shared_ptr<SYSCALL_FUNC> func);
    /**
     * Retrieves the syscall emulation function structure based on its numeric syscall number.
     * @param[in] sysno The numeric syscall number.
     * @return Shared pointer to the found `SYSCALL_FUNC`, or `nullptr` if not found.
     */
    std::shared_ptr<SYSCALL_FUNC> ARION_EXPORT get_syscall_func(uint64_t sysno);
    /**
     * Retrieves the syscall emulation function structure based on its string name.
     * @param[in] name The string name of the syscall.
     * @return Shared pointer to the found `SYSCALL_FUNC`, or `nullptr` if not found.
     */
    std::shared_ptr<SYSCALL_FUNC> ARION_EXPORT get_syscall_func(std::string name);
};

}; // namespace arion

#endif // ARION_LNX_SYSCALL_MANAGER_HPP
