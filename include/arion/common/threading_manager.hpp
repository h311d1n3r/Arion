#ifndef ARION_THREADING_MANAGER_HPP
#define ARION_THREADING_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <map>
#include <memory>
#include <stack>
#include <arion/unicorn/unicorn.h>
#include <vector>

class Arion;

struct ARION_FUTEX
{
    arion::ADDR futex_addr = 0;
    uint32_t futex_bitmask = 0;
    pid_t tid;
    ARION_FUTEX() {};
    ARION_FUTEX(arion::ADDR futex_addr, uint32_t futex_bitmask, pid_t tid)
        : futex_addr(futex_addr), futex_bitmask(futex_bitmask), tid(tid) {};
    ARION_FUTEX(ARION_FUTEX *arion_f)
        : futex_addr(arion_f->futex_addr), futex_bitmask(arion_f->futex_bitmask), tid(arion_f->tid) {};
};
std::vector<arion::BYTE> serialize_arion_futex(ARION_FUTEX *arion_f);
ARION_FUTEX *deserialize_arion_futex(std::vector<arion::BYTE> srz_thread);

struct ARION_EXPORT ARION_THREAD
{
    pid_t tid;
    pid_t tgid;
    int exit_signal;
    uint64_t flags;
    arion::ADDR child_tid_addr;
    arion::ADDR parent_tid_addr;
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> regs_state = nullptr;
    int64_t lock = 0;
    arion::ADDR wait_status_addr = 0;
    bool paused = false;
    arion::ADDR robust_list_head = 0;
    arion::ADDR rseq_addr = 0;
    uint32_t rseq_len = 0;
    uint32_t rseq_sig = 0;
    ARION_THREAD() {};
    ARION_THREAD(int exit_signal, uint64_t flags, arion::ADDR child_tid_addr, arion::ADDR parent_tid_addr,
                 std::unique_ptr<std::map<arion::REG, arion::RVAL>> regs_state)
        : exit_signal(exit_signal), flags(flags), child_tid_addr(child_tid_addr), parent_tid_addr(parent_tid_addr),
          regs_state(std::move(regs_state)) {};
    ARION_THREAD(ARION_THREAD *arion_t)
        : tid(arion_t->tid), tgid(arion_t->tgid), exit_signal(arion_t->exit_signal), flags(arion_t->flags),
          child_tid_addr(arion_t->child_tid_addr), parent_tid_addr(arion_t->parent_tid_addr),
          regs_state(arion_t->regs_state ? std::make_unique<std::map<arion::REG, arion::RVAL>>(*arion_t->regs_state)
                                         : nullptr),
          lock(arion_t->lock), wait_status_addr(arion_t->wait_status_addr), paused(arion_t->paused) {};
};
std::vector<arion::BYTE> serialize_arion_thread(ARION_THREAD *arion_t);
ARION_THREAD *deserialize_arion_thread(std::vector<arion::BYTE> srz_thread);

struct ARION_EXPORT ARION_TGROUP_ENTRY
{
    pid_t tid;
    pid_t tgid;
    pid_t pid;
    ARION_TGROUP_ENTRY(pid_t tid, pid_t tgid, pid_t pid) : tid(tid), tgid(tgid), pid(pid) {};
};

class ARION_EXPORT ThreadingManager
{
  private:
    static std::map<int, std::string> sync_signals;
    std::weak_ptr<Arion> arion;
    pid_t curr_id = 1;
    pid_t running_tid = 1;
    std::stack<pid_t> free_thread_ids;
    std::map<int, std::shared_ptr<struct ksigaction>> sighandlers;
    static void intr_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data);
    static bool invalid_memory_hook(std::shared_ptr<Arion> arion, uc_mem_type access, uint64_t addr, int size,
                                    int64_t val, void *user_data);
    static bool invalid_insn_hook(std::shared_ptr<Arion> arion, void *user_data);
    pid_t gen_next_id();
    void handle_sync_signals();
    void handle_async_signals();
    void handle_wait_signals();

  public:
    static std::map<pid_t, std::vector<std::unique_ptr<ARION_TGROUP_ENTRY>>> thread_groups;
    std::map<pid_t, std::unique_ptr<ARION_THREAD>> threads_map;
    std::map<arion::ADDR, std::vector<std::unique_ptr<ARION_FUTEX>> *> futex_list;
    std::map<pid_t, pid_t> sigwait_list;
    static std::unique_ptr<ThreadingManager> initialize(std::weak_ptr<Arion> arion);
    ThreadingManager(std::weak_ptr<Arion> arion);
    ~ThreadingManager();
    pid_t ARION_EXPORT add_thread_entry(std::unique_ptr<ARION_THREAD> thread);
    void ARION_EXPORT remove_thread_entry(pid_t tid);
    void ARION_EXPORT clear_threads();
    pid_t clone_thread(uint64_t flags, arion::ADDR new_sp, arion::ADDR new_tls, arion::ADDR child_tid_addr,
                       arion::ADDR parent_tid_addr, int exit_signal);
    void switch_to_thread(pid_t tid);
    void switch_to_next_thread();
    void futex_wait(pid_t tid, arion::ADDR futex_addr, uint32_t futex_bitmask);
    void futex_wait_curr(arion::ADDR futex_addr, uint32_t futex_bitmask);
    bool signal_wait(pid_t tid, pid_t target_pid);
    bool signal_wait_curr(pid_t target_pid);
    bool has_sighandler(int signo);
    std::shared_ptr<struct ksigaction> get_sighandler(int signo);
    void set_sighandler(int signo, std::shared_ptr<struct ksigaction> sighandler);
    void handle_signals();
    size_t futex_wake(arion::ADDR futex_addr, uint32_t futex_bitmask);
    bool ARION_EXPORT is_curr_locked();
    size_t ARION_EXPORT get_threads_count();
    pid_t ARION_EXPORT get_running_tid();
    void set_running_tid(pid_t tid);
    void set_tgid(pid_t tid, pid_t tgid, bool init = false);
};

#endif // ARION_THREADING_MANAGER_HPP
