#ifndef ARION_SIGNAL_MANAGER_HPP
#define ARION_SIGNAL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/unicorn/unicorn.h>
#include <map>
#include <memory>

namespace arion
{

class Arion;

class SignalManager
{
  private:
    std::weak_ptr<Arion> arion;
    std::map<int, std::shared_ptr<struct ksigaction>> sighandlers;
    std::map<pid_t, pid_t> sigwait_list;
    std::unique_ptr<std::map<REG, RVAL>> ucontext_regs;
    static std::map<int, std::string> signals;
    static void intr_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data);
    static bool invalid_memory_hook(std::shared_ptr<Arion> arion, uc_mem_type access, uint64_t addr, int size,
                                    int64_t val, void *user_data);
    static bool invalid_insn_hook(std::shared_ptr<Arion> arion, void *user_data);
    bool handle_sighandler(pid_t source_pid, int signo);
    void handle_sigchld(pid_t source_pid);

  public:
    static std::unique_ptr<SignalManager> initialize(std::weak_ptr<Arion> arion);
    SignalManager(std::weak_ptr<Arion> arion);
    void print_signal(std::shared_ptr<Arion> arion, std::string sig_name);
    void handle_signal(pid_t source_pid, int signo);
    void wait_for_sig(pid_t target_tid, pid_t source_pid, ADDR wait_status_addr);
    bool has_sighandler(int signo);
    std::shared_ptr<struct ksigaction> get_sighandler(int signo);
    void set_sighandler(int signo, std::shared_ptr<struct ksigaction> sighandler);
    bool sigreturn();
};

}; // namespace arion

#endif // ARION_SIGNAL_MANAGER_HPP
