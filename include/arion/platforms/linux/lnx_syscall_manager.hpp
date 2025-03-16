#ifndef ARION_LNX_SYSCALL_MANAGER_HPP
#define ARION_LNX_SYSCALL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>

class Arion;

using SYS_PARAM = uint64_t;
using SYSCALL_FUNC = std::function<uint64_t(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)>;

extern std::map<std::string, uint8_t> PARAMS_N_BY_SYSCALL_NAME;

class ARION_EXPORT LinuxSyscallManager
{
  private:
    std::weak_ptr<Arion> arion;
    std::map<uint64_t, SYSCALL_FUNC> syscall_funcs;
    void add_syscall_entry(std::string name, SYSCALL_FUNC func);
    void init_syscall_funcs();

  public:
    static std::unique_ptr<LinuxSyscallManager> initialize(std::weak_ptr<Arion> arion);
    LinuxSyscallManager(std::weak_ptr<Arion> arion);
    void process_syscall(std::shared_ptr<Arion> arion);
    void ARION_EXPORT set_syscall_func(uint64_t sysno, SYSCALL_FUNC func);
    void ARION_EXPORT set_syscall_func(std::string name, SYSCALL_FUNC func);
    SYSCALL_FUNC ARION_EXPORT get_syscall_func(uint64_t sysno);
    SYSCALL_FUNC ARION_EXPORT get_syscall_func(std::string name);
};

#endif // ARION_LNX_SYSCALL_MANAGER_HPP
