#ifndef ARION_LNX_SYSCALL_MANAGER_HPP
#define ARION_LNX_SYSCALL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/utils/type_utils.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>

class Arion;

struct SYSCALL_FUNC
{
    std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params, bool &cancel)> func;
    std::vector<std::shared_ptr<ArionType>> signature;

    SYSCALL_FUNC(
        std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params, bool &cancel)> func,
        std::vector<std::shared_ptr<ArionType>> signature)
        : func(func), signature(signature) {};
};

extern std::map<std::string, uint8_t> PARAMS_N_BY_SYSCALL_NAME;

class ARION_EXPORT LinuxSyscallManager
{
  private:
    std::weak_ptr<Arion> arion;
    std::map<uint64_t, std::shared_ptr<SYSCALL_FUNC>> syscall_funcs;
    void add_syscall_entry(std::string name, std::shared_ptr<SYSCALL_FUNC> func);
    template <typename... SignatureArgs>
    std::shared_ptr<SYSCALL_FUNC> make_sys_func(
        std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<arion::SYS_PARAM> params, bool &cancel)> func,
        SignatureArgs &&...signature)
    {
        std::vector<std::shared_ptr<ArionType>> sig_vec = {std::forward<SignatureArgs>(signature)...};
        return std::make_shared<SYSCALL_FUNC>(func, sig_vec);
    }
    void init_syscall_funcs();
    void print_syscall(std::shared_ptr<Arion> arion, std::string sys_name,
                       std::vector<std::shared_ptr<ArionType>> signature, std::vector<arion::SYS_PARAM> func_params,
                       uint64_t syscall_ret);

  public:
    static std::unique_ptr<LinuxSyscallManager> initialize(std::weak_ptr<Arion> arion);
    LinuxSyscallManager(std::weak_ptr<Arion> arion);
    void process_syscall(std::shared_ptr<Arion> arion);
    void ARION_EXPORT set_syscall_func(uint64_t sysno, std::shared_ptr<SYSCALL_FUNC> func);
    void ARION_EXPORT set_syscall_func(std::string name, std::shared_ptr<SYSCALL_FUNC> func);
    std::shared_ptr<SYSCALL_FUNC> ARION_EXPORT get_syscall_func(uint64_t sysno);
    std::shared_ptr<SYSCALL_FUNC> ARION_EXPORT get_syscall_func(std::string name);
};

#endif // ARION_LNX_SYSCALL_MANAGER_HPP
