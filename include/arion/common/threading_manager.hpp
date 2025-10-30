#ifndef ARION_THREADING_MANAGER_HPP
#define ARION_THREADING_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/unicorn/unicorn.h>
#include <map>
#include <memory>
#include <stack>
#include <vector>

namespace arion
{

class Arion;

/// This structure holds data about a Fast Userspace muTEX.
struct ARION_FUTEX
{
    /// Memory address of the futex.
    ADDR futex_addr = 0;
    /// Bitmask of the futex, used to encode various states or conditions.
    uint32_t futex_bitmask = 0;
    /// The Thread ID associated with the futex.
    pid_t tid;
    /**
     * Builder for ARION_FUTEX instances.
     */
    ARION_FUTEX() {};
    /**
     * Builder for ARION_FUTEX instances.
     * @param[in] futex_addr Memory address of the futex.
     * @param[in] futex_bitmask Bitmask of the futex, used to encode various states or conditions.
     * @param[in] tid The Thread ID associated with the futex.
     */
    ARION_FUTEX(ADDR futex_addr, uint32_t futex_bitmask, pid_t tid)
        : futex_addr(futex_addr), futex_bitmask(futex_bitmask), tid(tid) {};
    /**
     * Builder for ARION_FUTEX instances used to clone an ARION_FUTEX instance.
     * @param[in] arion_f The ARION_FUTEX instance to be cloned.
     */
    ARION_FUTEX(ARION_FUTEX *arion_f)
        : futex_addr(arion_f->futex_addr), futex_bitmask(arion_f->futex_bitmask), tid(arion_f->tid) {};
};
/*
 * Serializes an ARION_FUTEX instance into a vector of bytes.
 * @param[in] arion_f The ARION_FUTEX to be serialized.
 * @return The serialized vector of bytes.
 */
std::vector<BYTE> serialize_arion_futex(ARION_FUTEX *arion_f);
/*
 * Deserializes an ARION_FUTEX instance from a vector of bytes.
 * @param[in] srz_futex The serialized vector of bytes.
 * @return The deserialized ARION_FUTEX.
 */
ARION_FUTEX *deserialize_arion_futex(std::vector<BYTE> srz_futex);

/// This structure holds data about a UNIX thread.
struct ARION_EXPORT ARION_THREAD
{
    /// The thread ID (TID) associated with this thread.
    pid_t tid;
    /// The thread group ID (TGID) associated with this thread.
    pid_t tgid;
    /// The signal to be sent to the parent process when the thread terminates.
    int exit_signal;
    /// The clone flags used when the thread was created.
    uint64_t flags;
    /// The address of the child_tid variable to be cleared when the thread exits (used with CLONE_CHILD_CLEARTID).
    ADDR child_cleartid_addr;
    /// The address of the child_tid variable to be set when the thread starts (used with CLONE_CHILD_SETTID).
    ADDR child_settid_addr;
    /// The address in the parent process where the TID of the newly created thread is stored (used with
    /// CLONE_PARENT_SETTID).
    ADDR parent_tid_addr;
    /// The saved register state of the thread, mapping each Unicorn register (REG) to its corresponding value (RVAL).
    std::unique_ptr<std::map<REG, RVAL>> regs_state = nullptr;
    /// The address of the thread’s thread-local storage (TLS) block.
    ADDR tls_addr;
    /// The address where the thread’s wait status is stored, used for synchronization with other threads.
    ADDR wait_status_addr = 0;
    /// Indicates whether the thread is currently stopped.
    bool stopped = false;
    /// The head of the robust futex list, used for robust mutex handling by the Linux kernel.
    ADDR robust_list_head = 0;
    /// The address of the restartable sequences (rseq) structure for this thread.
    ADDR rseq_addr = 0;
    /// The size of the restartable sequences (rseq) area in bytes.
    uint32_t rseq_len = 0;
    /// The signature used to identify aborted restartable sequences.
    uint32_t rseq_sig = 0;
    /**
     * Builder for ARION_THREAD instances.
     */
    ARION_THREAD() {};
    /**
     * Builder for ARION_THREAD instances.
     * @param[in] exit_signal The signal to be sent to the parent process when the thread terminates.
     * @param[in] flags The clone flags used when the thread was created.
     * @param[in] child_cleartid_addr The address of the child_tid variable to be cleared when the thread exits (used
     * with CLONE_CHILD_CLEARTID).
     * @param[in] child_settid_addr The address of the child_tid variable to be set when the thread starts (used with
     * CLONE_CHILD_SETTID).
     * @param[in] parent_tid_addr The address in the parent process where the TID of the newly created thread is stored
     * (used with CLONE_PARENT_SETTID).
     * @param[in] regs_state The saved register state of the thread, mapping each Unicorn register (REG) to its
     * corresponding value (RVAL).
     * @param[in] tls_addr The address of the thread’s thread-local storage (TLS) block.
     */
    ARION_THREAD(int exit_signal, uint64_t flags, ADDR child_cleartid_addr, ADDR child_settid_addr,
                 ADDR parent_tid_addr, std::unique_ptr<std::map<REG, RVAL>> regs_state, ADDR tls_addr)
        : exit_signal(exit_signal), flags(flags), child_cleartid_addr(child_cleartid_addr),
          child_settid_addr(child_settid_addr), parent_tid_addr(parent_tid_addr), regs_state(std::move(regs_state)),
          tls_addr(tls_addr) {};
    /**
     * Builder for ARION_THREAD instances used to clone an ARION_THREAD instance.
     * @param[in] arion_t The ARION_THREAD instance to be cloned.
     */
    ARION_THREAD(ARION_THREAD *arion_t)
        : tid(arion_t->tid), tgid(arion_t->tgid), exit_signal(arion_t->exit_signal), flags(arion_t->flags),
          child_cleartid_addr(arion_t->child_cleartid_addr), child_settid_addr(arion_t->child_settid_addr),
          parent_tid_addr(arion_t->parent_tid_addr),
          regs_state(arion_t->regs_state ? std::make_unique<std::map<REG, RVAL>>(*arion_t->regs_state) : nullptr),
          tls_addr(arion_t->tls_addr), wait_status_addr(arion_t->wait_status_addr), stopped(arion_t->stopped) {};
};
/*
 * Serializes an ARION_THREAD instance into a vector of bytes.
 * @param[in] arion_t The ARION_THREAD to be serialized.
 * @return The serialized vector of bytes.
 */
std::vector<BYTE> serialize_arion_thread(ARION_THREAD *arion_t);
/*
 * Deserializes an ARION_THREAD instance from a vector of bytes.
 * @param[in] srz_thread The serialized vector of bytes.
 * @return The deserialized ARION_THREAD.
 */
ARION_THREAD *deserialize_arion_thread(std::vector<BYTE> srz_thread);

/// This structure stores the identifiers associated with a thread and its parent process within a thread group,
/// allowing Arion to track relationships between threads and processes.
struct ARION_EXPORT ARION_TGROUP_ENTRY
{
    /// The Thread ID (TID) of the thread.
    pid_t tid;
    /// The Thread Group ID (TGID) associated with the thread.
    pid_t tgid;
    /// The Process ID (PID) associated with the thread.
    pid_t pid;
    /**
     * Builder for ARION_TGROUP_ENTRY instances.
     * @param[in] tid The Thread ID (TID) of the thread.
     * @param[in] tgid The Thread Group ID (TGID) associated with the thread.
     * @param[in] pid The Process ID (PID) associated with the thread.
     */
    ARION_TGROUP_ENTRY(pid_t tid, pid_t tgid, pid_t pid) : tid(tid), tgid(tgid), pid(pid) {};
};

/// This class is used to manage threading operations. It is responsible for creating, managing, and synchronizing
/// threads and processes.
class ARION_EXPORT ThreadingManager
{
  private:
    /// The Arion instance associated with this instance.
    std::weak_ptr<Arion> arion;
    /// The next thread ID to assign when creating new threads.
    pid_t curr_id = 1;
    /// The thread ID of the currently running thread.
    pid_t running_tid = 1;
    /// Stack of reusable thread IDs from previously terminated threads.
    std::stack<pid_t> free_thread_ids;
    /**
     * Generates the next available Thread ID.
     * @return The next unique Thread ID.
     */
    pid_t gen_next_id();
    /**
     * Removes a thread entry, used internally by other methods.
     * @param[in] tid The Thread ID to remove.
     * @param[in] clearing Whether this removal is part of a global clear operation.
     */
    void remove_thread_entry_internal(pid_t tid, bool clearing = false);

  public:
    /// A map identifying a thread group given its associated Thread ID.
    static std::map<pid_t, std::vector<std::unique_ptr<ARION_TGROUP_ENTRY>>> thread_groups;
    /// A map identifying an ARION_THREAD instance given its associated Thread ID.
    std::map<pid_t, std::unique_ptr<ARION_THREAD>> threads_map;
    /// A map identifying an ARION_FUTEX instance given the address of the futex.
    std::map<ADDR, std::vector<std::unique_ptr<ARION_FUTEX>> *> futex_list;
    /**
     * Instanciates and initializes new ThreadingManager objects with some parameters.
     * @param[in] arion The Arion instance associated with this instance.
     * @return A new ThreadingManager instance.
     */
    static std::unique_ptr<ThreadingManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for ThreadingManager instances.
     * @param[in] arion The Arion instance associated with this instance.
     */
    ThreadingManager(std::weak_ptr<Arion> arion);
    /**
     * Destructor for ThreadingManager instances.
     */
    ~ThreadingManager();
    /**
     * Adds a new thread entry to the internal tracking structures.
     * @param[in] thread The ARION_THREAD instance to register.
     * @return The newly assigned thread ID (TID).
     */
    pid_t ARION_EXPORT add_thread_entry(std::unique_ptr<ARION_THREAD> thread);
    /**
     * Removes a thread entry from the ThreadingManager.
     * @param[in] tid The thread ID to remove.
     */
    void ARION_EXPORT remove_thread_entry(pid_t tid);
    /**
     * Removes all thread entries and clears the internal structures.
     */
    void ARION_EXPORT clear_threads();
    /**
     * Creates a new thread by emulating the clone() system call.
     * @param[in] flags Clone flags defining thread sharing behavior.
     * @param[in] new_sp New stack pointer for the thread.
     * @param[in] new_tls New TLS (Thread-Local Storage) address.
     * @param[in] child_tid_addr Address where the child TID will be written.
     * @param[in] parent_tid_addr Address where the parent TID will be written.
     * @param[in] exit_signal Signal to send when the thread exits.
     * @return The newly created thread's TID.
     */
    pid_t clone_thread(uint64_t flags, ADDR new_sp, ADDR new_tls, ADDR child_tid_addr, ADDR parent_tid_addr,
                       int exit_signal);
    /**
     * Creates a new process by emulating the fork() system call.
     * @param[in] flags Fork flags defining process creation behavior.
     * @param[in] new_sp New stack pointer for the process.
     * @param[in] new_tls New TLS (Thread-Local Storage) address.
     * @param[in] child_tid_addr Address where the child TID will be written.
     * @param[in] parent_tid_addr Address where the parent TID will be written.
     * @param[in] exit_signal Signal to send when the process exits.
     * @return The newly created process's PID.
     */
    pid_t fork_process(uint64_t flags, ADDR new_sp, ADDR new_tls, ADDR child_tid_addr, ADDR parent_tid_addr,
                       int exit_signal);
    /**
     * Switches execution context to the thread identified by `tid`.
     * @param[in] tid The target thread ID to switch to.
     */
    void switch_to_thread(pid_t tid);
    /**
     * Switches execution context to the next available thread.
     */
    void switch_to_next_thread();
    /**
     * Makes the specified thread wait on a futex.
     * @param[in] tid The thread ID that will wait.
     * @param[in] futex_addr Address of the futex being waited on.
     * @param[in] futex_bitmask Bitmask used for wake filtering.
     */
    void futex_wait(pid_t tid, ADDR futex_addr, uint32_t futex_bitmask);
    /**
     * Makes the currently running thread wait on a futex.
     * @param[in] futex_addr Address of the futex being waited on.
     * @param[in] futex_bitmask Bitmask used for wake filtering.
     */
    void futex_wait_curr(ADDR futex_addr, uint32_t futex_bitmask);
    /**
     * Makes a target thread wait for a signal from a source process.
     * @param[in] target_tid The thread ID that will wait.
     * @param[in] source_pid The PID of the process sending the signal.
     * @param[in] wait_status_addr Address where the wait status will be stored.
     * @return True if the wait was successful, false otherwise.
     */
    bool signal_wait(pid_t target_tid, pid_t source_pid, ADDR wait_status_addr);
    /**
     * Makes the currently running thread wait for a signal from a source process.
     * @param[in] source_pid The PID of the process sending the signal.
     * @param[in] wait_status_addr Address where the wait status will be stored.
     * @return True if the wait was successful, false otherwise.
     */
    bool signal_wait_curr(pid_t source_pid, ADDR wait_status_addr);
    /**
     * Wakes up threads waiting on a futex.
     * @param[in] futex_addr Address of the futex being signaled.
     * @param[in] futex_bitmask Bitmask used for filtering which threads to wake.
     * @return The number of threads woken up.
     */
    size_t futex_wake(ADDR futex_addr, uint32_t futex_bitmask);
    /**
     * Checks whether the currently running thread is locked.
     * @return True if the thread is locked, false otherwise.
     */
    bool ARION_EXPORT is_curr_locked();
    /**
     * Retrieves the total number of threads currently managed by this instance.
     * @return The number of active threads.
     */
    size_t ARION_EXPORT get_threads_count();
    /**
     * Retrieves the ID of the currently running thread.
     * @return The running thread's TID.
     */
    pid_t ARION_EXPORT get_running_tid();
    /**
     * Sets the currently running thread ID.
     * @param[in] tid The TID of the thread to mark as running.
     */
    void set_running_tid(pid_t tid);
    /**
     * Sets the thread group ID (TGID) for a specific thread.
     * @param[in] tid The thread ID.
     * @param[in] tgid The thread group ID.
     * @param[in] init Whether this is part of an initialization sequence.
     */
    void set_tgid(pid_t tid, pid_t tgid, bool init = false);
    /**
     * Sets the same thread group ID (TGID) for all managed threads.
     * @param[in] tgid The thread group ID to assign.
     * @param[in] init Whether this is part of an initialization sequence.
     */
    void set_all_tgid(pid_t tgid, bool init = false);
};

}; // namespace arion

#endif // ARION_THREADING_MANAGER_HPP
