#ifndef ARION_SIGNAL_MANAGER_HPP
#define ARION_SIGNAL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <arion/unicorn/unicorn.h>
#include <map>
#include <memory>

namespace arion
{

class Arion;

/// This class manages signal handling within an Arion emulation session.
class SignalManager
{
  private:
    /// The Arion instance associated with this instance.
    std::weak_ptr<Arion> arion;
    /// A map identifying a ksigaction structure given its associated signal number.
    std::map<int, std::shared_ptr<struct arion_lnx_type::ksigaction>> sighandlers;
    /// A map identifying a process (ID) given the process (ID) it is waiting for.
    std::map<pid_t, pid_t> sigwait_list;
    /// A map identifying a saved register value to be restored, given the Unicorn register it is associated to.
    std::unique_ptr<std::map<REG, RVAL>> ucontext_regs;
    /// A map identifying a signal description string given its signal number.
    static std::map<int, std::string> signals;
    /**
     * A hook callback which is triggered when an interrupt occurs.
     * @param[in] arion The Arion instance that triggered the hook.
     * @param[in] intno The interrupt number.
     * @param[in] user_data Optional user-defined data passed to the hook.
     */
    static void intr_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data);
    /**
     * A hook callback which is triggered when a memory access violation occurs.
     * @param[in] arion The Arion instance that triggered the hook.
     * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
     * @param[in] addr Memory address being accessed.
     * @param[in] size Size of the memory access.
     * @param[in] val Value attempted to be written.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return True if the hook handled the access and default behavior should be suppressed.
     */
    static bool invalid_memory_hook(std::shared_ptr<Arion> arion, uc_mem_type access, uint64_t addr, int size,
                                    int64_t val, void *user_data);
    /**
     * A hook callback which is triggered when an invalid instruction gets fetched.
     * @param[in] arion The Arion instance that triggered the hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return True if the hook handled the access and default behavior should be suppressed.
     */
    static bool invalid_insn_hook(std::shared_ptr<Arion> arion, void *user_data);
    /**
     * If a sighandler is associated with a given signal for a given process, triggers its execution.
     * @param[in] source_pid The Process ID that generated the signal.
     * @param[in] signo The signal number.
     * @return True if a sighandler was found and executed for the given parameters.
     */
    bool handle_sighandler(pid_t source_pid, int signo);
    /**
     * Handles the specific case of SIGCHLD signals where a process waiting for another can be resumed.
     * @param[in] source_pid The Process ID that generated the SIGCHLD signal.
     */
    void handle_sigchld(pid_t source_pid);

  public:
    /**
     * Instanciates and initializes new SignalManager objects with some parameters.
     * @param[in] arion The Arion instance associated with this instance.
     * @return A new SignalManager instance.
     */
    static std::unique_ptr<SignalManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for SignalManager instances.
     * @param[in] arion The Arion instance associated with this instance.
     */
    SignalManager(std::weak_ptr<Arion> arion);
    /**
     * Logs a signal given its name.
     * @param[in] arion The Arion instance which triggered the signal.
     * @param[in] sig_name Name of the signal.
     */
    void print_signal(std::shared_ptr<Arion> arion, std::string sig_name);
    /**
     * Processes a signal after it has been triggered by a given process.
     * @param[in] source_pid The process that triggered the signal.
     * @param[in] signo The signal number.
     */
    void handle_signal(pid_t source_pid, int signo);
    /**
     * Sets a thread in a paused state, waiting for a given process to trigger a signal before resuming its execution.
     * @param[in] target_tid The thread waiting for a given process.
     * @param[in] source_pid The process which must trigger a signal to resume the thread execution.
     * @param[in] wait_status_addr The address where to store the exit status of the signaled process.
     */
    void wait_for_sig(pid_t target_tid, pid_t source_pid, ADDR wait_status_addr);
    /**
     * Checks whether a given signal has a sighandler being attached.
     * @param[in] signo The signal number.
     * @return True if the given signal has a sighandler being attached.
     */
    bool has_sighandler(int signo);
    /**
     * Retrieves a sighandler from a given signal.
     * @param[in] signo The signal number.
     * @return The sighandler.
     */
    std::shared_ptr<struct arion_lnx_type::ksigaction> get_sighandler(int signo);
    /**
     * Associates a sighandler to a given signal.
     * @param[in] signo The signal number.
     * @param[in] sighandler The sighandler.
     */
    void set_sighandler(int signo, std::shared_ptr<struct arion_lnx_type::ksigaction> sighandler);
    /**
     * Returns from a sighandler by restoring context.
     * @return True if a context could be restored.
     */
    bool sigreturn();
};

}; // namespace arion

#endif // ARION_SIGNAL_MANAGER_HPP
