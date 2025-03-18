#ifndef ARION_ARION_HPP
#define ARION_ARION_HPP

#include <arion/archs/x86/gdt_manager.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/common/code_tracer.hpp>
#include <arion/common/context_manager.hpp>
#include <arion/common/file_system_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/common/logger.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/common/socket_manager.hpp>
#include <arion/common/threading_manager.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_syscall_manager.hpp>
#include <arion/capstone/capstone.h>
#include <exception>
#include <arion/keystone/keystone.h>
#include <memory>
#include <string>
#include <arion/unicorn/unicorn.h>
#include <vector>

namespace arion
{
extern std::map<arion::CPU_ARCH, std::pair<uc_arch, uc_mode>> ARION_TO_UC_ARCH;
extern std::map<arion::CPU_ARCH, std::vector<std::pair<ks_arch, ks_mode>>> ARION_TO_KS_ARCH;
extern std::map<arion::CPU_ARCH, std::vector<std::pair<cs_arch, cs_mode>>> ARION_TO_CS_ARCH;
}; // namespace arion

class ARION_EXPORT Arion : public std::enable_shared_from_this<Arion>
{
  private:
    static pid_t NEXT_PID;
    static std::map<pid_t, std::weak_ptr<Arion>> instances;
    std::vector<std::string> program_args;
    std::vector<std::string> program_env;
    std::weak_ptr<Arion> parent;
    std::vector<std::shared_ptr<Arion>> children;
    std::exception_ptr uc_exception = nullptr;
    bool running = false;
    bool sync = false;
    pid_t pid;
    pid_t pgid;
    void init_engines(arion::CPU_ARCH arch);
    void init_program(std::shared_ptr<ElfParser> prog_parser);
    void init_dynamic_program(std::shared_ptr<ElfParser> prog_parser);
    void init_static_program(std::shared_ptr<ElfParser> prog_parser);
    void close_engines();
    bool run_current(bool multi_process, bool main_inst);
    void run_children();

  public:
    std::unique_ptr<AbiManager> abi;
    std::unique_ptr<MemoryManager> mem;
    std::unique_ptr<FileSystemManager> fs;
    std::unique_ptr<SocketManager> sock;
    std::unique_ptr<HooksManager> hooks;
    std::unique_ptr<ThreadingManager> threads;
    std::unique_ptr<ContextManager> context;
    std::unique_ptr<LinuxSyscallManager> syscalls;
    std::unique_ptr<GdtManager> gdt_manager;
    std::unique_ptr<CodeTracer> tracer;
    std::unique_ptr<Logger> logger;
    std::unique_ptr<LOADER_PARAMS> loader_params;
    std::vector<std::shared_ptr<arion::SIGNAL>> pending_signals;
    uc_engine *uc;
    std::vector<ks_engine *> ks;
    std::vector<csh *> cs;
    bool is_zombie = false;
    static std::shared_ptr<Arion> ARION_EXPORT
    new_instance(std::vector<std::string> program_args, std::string fs_path = "/",
                 std::vector<std::string> program_env = std::vector<std::string>(), std::string cwd = "",
                 ARION_LOG_LEVEL log_lvl = ARION_LOG_LEVEL::INFO, pid_t pid = 0);
    static std::map<pid_t, std::weak_ptr<Arion>> ARION_EXPORT get_arion_instances();
    static bool ARION_EXPORT has_arion_instance(pid_t pid);
    static std::weak_ptr<Arion> ARION_EXPORT get_arion_instance(pid_t pid);
    ~Arion();
    void cleanup_process();
    void ARION_EXPORT run();
    void ARION_EXPORT stop();
    void crash(std::exception_ptr exception);
    void ARION_EXPORT sync_threads();
    bool ARION_EXPORT is_running();
    std::shared_ptr<Arion> copy();
    bool ARION_EXPORT has_parent();
    std::shared_ptr<Arion> ARION_EXPORT get_parent();
    std::vector<std::shared_ptr<Arion>> ARION_EXPORT get_children();
    std::vector<std::shared_ptr<Arion>> ARION_EXPORT get_pgid_children(pid_t pgid);
    pid_t ARION_EXPORT add_child(std::shared_ptr<Arion> child);
    bool ARION_EXPORT has_child(pid_t child_pid);
    bool ARION_EXPORT is_child_of(pid_t parent_pid);
    std::shared_ptr<Arion> ARION_EXPORT get_child(pid_t child_pid);
    void ARION_EXPORT replace_child(pid_t child_pid, std::shared_ptr<Arion> child);
    void ARION_EXPORT remove_child(pid_t child_pid);
    void execve(std::string file_path, std::vector<std::string> argv, std::vector<std::string> envp);
    std::vector<std::string> ARION_EXPORT get_program_args();
    std::vector<std::string> ARION_EXPORT get_program_env();
    pid_t ARION_EXPORT get_pid();
    pid_t ARION_EXPORT get_pgid();
    void ARION_EXPORT set_pgid(pid_t pgid);
    void ARION_EXPORT send_signal(pid_t source_pid, int signo);
};

#endif // ARION_ARION_HPP
