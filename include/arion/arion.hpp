#ifndef ARION_ARION_HPP
#define ARION_ARION_HPP

#include <arion/archs/x86/gdt_manager.hpp>
#include <arion/capstone/capstone.h>
#include <arion/common/arch_manager.hpp>
#include <arion/common/baremetal_manager.hpp>
#include <arion/common/code_tracer.hpp>
#include <arion/common/config.hpp>
#include <arion/common/context_manager.hpp>
#include <arion/common/file_system_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/common/logger.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/common/signal_manager.hpp>
#include <arion/common/socket_manager.hpp>
#include <arion/common/threading_manager.hpp>
#include <arion/keystone/keystone.h>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_syscall_manager.hpp>
#include <arion/unicorn/unicorn.h>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace arion
{
/// Ensures the conversion between Arion CPU_ARCH and Unicorn CPU architectures and modes.
extern std::map<CPU_ARCH, std::pair<uc_arch, uc_mode>> ARION_TO_UC_ARCH;
/// Ensures the conversion between Arion CPU_ARCH and Keystone CPU architectures and modes.
extern std::map<CPU_ARCH, std::vector<std::pair<ks_arch, ks_mode>>> ARION_TO_KS_ARCH;
/// Ensures the conversion between Arion CPU_ARCH and Capstone CPU architectures and modes.
extern std::map<CPU_ARCH, std::vector<std::pair<cs_arch, cs_mode>>> ARION_TO_CS_ARCH;

/// Group of Arion instances that run together. This allows IPC as well as sandboxing between groups.
class ARION_EXPORT ArionGroup : public std::enable_shared_from_this<ArionGroup>
{
  private:
    /// True when emulation of the group should be stopped.
    bool trigger_stop;
    /// Next PID to be associated with an Arion instance, in case none is provided when inserting it in
    /// the group.
    pid_t next_pid = ARION_PROCESS_PID;
    /// PID of the Arion instance currently running in the group.
    pid_t curr_pid = ARION_PROCESS_PID;
    /// List of Arion instances associated with this group.
    std::map<pid_t, std::shared_ptr<Arion>> instances;

  public:
    /**
     * Returns the amount of Arion instances associated with this group.
     * @return The amount.
     */
    size_t ARION_EXPORT count_arion_instances();
    /**
     * Provides the map of all Arion instances associated with this group, with their respective PID being the keys.
     * @return The map.
     */
    std::map<pid_t, std::shared_ptr<Arion>> ARION_EXPORT get_arion_instances();
    /**
     * Checks whether a given PID is associated with an Arion instance in this group or not.
     * @param[in] pid The PID to check for.
     * @return True if the PID was found, false otherwise.
     */
    bool ARION_EXPORT has_arion_instance(pid_t pid);
    /**
     * Provides the Arion instance in this group associated with the given PID.
     * @param[in] pid The PID.
     * @return The Arion instance.
     */
    std::shared_ptr<Arion> ARION_EXPORT get_arion_instance(pid_t pid);
    /**
     * Inserts an Arion instance in this group.
     * @param[in] arion The Arion instance.
     * @param[in] pid The PID for this instance (optional, nullable).
     * @param[in] pgid The PGID for this instance (optional, nullable). Set to the PID value if not specified.
     */
    void ARION_EXPORT add_arion_instance(std::shared_ptr<Arion> arion, std::optional<pid_t> pid = std::nullopt,
                                         std::optional<pid_t> pgid = std::nullopt);
    /**
     * Removes an Arion instance from this group, given its PID.
     * @param[in] pid The PID associated with the Arion instance to remove.
     */
    void ARION_EXPORT remove_arion_instance(pid_t pid);
    /**
     * Starts emulation of the group. Each Arion instance is given some execution time before switching to the next one.
     */
    void ARION_EXPORT run();
    /**
     * Stops emulation of the whole group.
     */
    void ARION_EXPORT stop();
    /**
     * Stops emulation of the running Arion instance in this group.
     */
    void ARION_EXPORT stop_curr();
    /**
     * Provides PID of running Arion instance in this group.
     * @return The PID.
     */
    pid_t ARION_EXPORT get_curr_pid();
    /**
     * Define which Arion instance should be currently run in the group, given its PID.
     * @param[in] The PID.
     */
    void set_curr_pid(pid_t pid);
    /**
     * Provides the next PID to be associated with an Arion instance, in case none is provided when inserting it in the
     * group.
     * @return The PID.
     */
    pid_t get_next_pid();
    /**
     * Defines the next PID to be associated with an Arion instance, in case none is provided when inserting it in the
     * group.
     * @param pid The PID.
     */
    void set_next_pid(pid_t pid);
};

/// An emulation unit associated with a process.
class ARION_EXPORT Arion : public std::enable_shared_from_this<Arion>
{
  private:
    /// The program arguments of the emulated process, basically the "argv" array.
    std::vector<std::string> program_args;
    /// The program environment variables of the emulated process, basically the "envp" array.
    std::vector<std::string> program_env;
    /// The ArionGroup this process is associated with. Arion instances are emulated together through their group.
    std::weak_ptr<ArionGroup> group;
    /// The Arion instance emulating the parent process of this process.
    std::weak_ptr<Arion> parent;
    /// The Arion instances emulating the child processes of this process.
    std::vector<std::weak_ptr<Arion>> children;
    /// An Unicorn exception that occurred during execution and needs to be rethrown.
    std::exception_ptr uc_exception = nullptr;
    /// True if this Arion instance is being run through the ArionAfl component.
    bool afl_mode = false;
    /// List of UNIX signals that must be considered as process crashes when fuzzing with ArionAfl component.
    std::vector<int> afl_signals;
    /// Start address for the emulation.
    std::optional<ADDR> start = std::nullopt;
    /// End address for the emulation.
    std::optional<ADDR> end = std::nullopt;
    /// True if this instance is currently being run by its ArionGroup.
    bool running = false;
    /// True if the emulation was stopped in order to synchronize the states of its threads.
    bool sync = false;
    /// True if the emulation is currently stopped.
    bool stopped = false;
    /// True if the emulation has ended for this instance but not for its parent.
    bool zombie = false;
    /// The PID associated with the emulated process.
    pid_t pid = 0;
    /// The PGID associated with the emulated process.
    pid_t pgid = 0;
    /// The SID associated with the emulated process.
    uint32_t sid = 0;
    /// The UID associated with the emulated process.
    uint32_t uid = 0;
    /// The GID associated with the emulated process.
    uint32_t gid = 0;
    /// The EUID associated with the emulated process.
    uint32_t euid = 0;
    /// The EGID associated with the emulated process.
    uint32_t egid = 0;
    /**
     * Performs the first steps to initialize this Arion instance, no matter the nature of the emulated process.
     * @param[in] arion The Arion instance being initialized.
     * @param[in] fs_path Path to the root directory for the file system of the emulated process (optional).
     * @param[in] program_env The program environment variables of the emulated process (optional), basically the "envp"
     * array.
     * @param[in] cwd The current working directory for the emulated process (optional).
     * @param[in] config An instance of Config associated with this Arion instance (optional).
     */
    static void new_instance_common_init(std::shared_ptr<Arion> arion, std::string fs_path = "/",
                                         std::vector<std::string> program_env = std::vector<std::string>(),
                                         std::string cwd = "",
                                         std::unique_ptr<Config> config = std::move(std::make_unique<Config>()));
    /**
     * Performs the final steps to initialize this Arion instance, no matter the nature of the emulated process.
     * @param[in] arion The arion instance being initialized.
     * @param[in] arch The CPU architecture to be emulated for this process.
     */
    static void new_instance_common_finish(std::shared_ptr<Arion> arion, CPU_ARCH arch);
    /**
     * Performs the initialization of engines related to CPU emulation, assembly and disassembly.
     * @param[in] The CPU architecture to be emulated for this process.
     */
    void init_engines(CPU_ARCH arch);
    /**
     * Performs the initialization steps specific to a file based process (executable).
     * @param[in] prog_parser The ExecutableParser instance that analyzed the program file.
     */
    void init_file_program(std::shared_ptr<ExecutableParser> prog_parser);
    /**
     * Performs the initialization steps specific to a baremetal process.
     */
    void init_baremetal_program();
    /**
     * Performs the initialization steps specific to a dynamic process.
     * @param[in] prog_parser The ExecutableParser instance that analyzed the program file.
     */
    void init_dynamic_program(std::shared_ptr<ExecutableParser> prog_parser);
    /**
     * Performs the initialization steps specific to a static process.
     * @param[in] prog_parser The ExecutableParser instance that analyzed the program file.
     */
    void init_static_program(std::shared_ptr<ExecutableParser> prog_parser);
    /**
     * Performs the destruction of engines related to CPU emulation, assembly and disassembly.
     */
    void close_engines();

  public:
    /// ArchManager instance associated with this instance.
    std::unique_ptr<ArchManager> arch;
    /// BaremetalManager instance associated with this instance.
    std::unique_ptr<BaremetalManager> baremetal;
    /// MemoryManager instance associated with this instance.
    std::unique_ptr<MemoryManager> mem;
    /// FileSystemManager instance associated with this instance.
    std::unique_ptr<FileSystemManager> fs;
    /// SocketManager instance associated with this instance.
    std::unique_ptr<SocketManager> sock;
    /// HooksManager instance associated with this instance.
    std::unique_ptr<HooksManager> hooks;
    /// ThreadingManager instance associated with this instance.
    std::unique_ptr<ThreadingManager> threads;
    /// SignalManager instance associated with this instance.
    std::unique_ptr<SignalManager> signals;
    /// ContextManager instance associated with this instance.
    std::unique_ptr<ContextManager> context;
    /// LinuxSyscallManager instance associated with this instance.
    std::unique_ptr<LinuxSyscallManager> syscalls;
    /// GdtManager instance associated with this instance.
    std::unique_ptr<GdtManager> gdt_manager;
    /// CodeTracer instance associated with this instance.
    std::unique_ptr<CodeTracer> tracer;
    /// Logger instance associated with this instance.
    std::unique_ptr<Logger> logger;
    /// Config instance associated with this instance.
    std::unique_ptr<Config> config;
    /// Parameters related to the loader action.
    std::unique_ptr<LNX_LOADER_PARAMS> loader_params;
    /// List of signals to be processed by this instance.
    std::vector<std::shared_ptr<SIGNAL>> pending_signals;
    /// Unicorn engine related to this instance.
    uc_engine *uc;
    /// Keystone engine related to this instance.
    std::vector<ks_engine *> ks;
    /// Capstone engine related to this instance.
    std::vector<csh *> cs;
    /**
     * Initialization method of Arion instances for file programs (executables).
     * @param[in] program_args The program arguments of the emulated process, basically the "argv" array.
     * @param[in] fs_path Path to the root directory for the file system of the emulated process (optional).
     * @param[in] program_env The program environment variables of the emulated process, basically the "envp" array
     * (optional).
     * @param[in] cwd The current working directory for the emulated process (optional).
     * @param[in] config An instance of Config associated with this Arion instance (optional).
     * @return The initialized Arion instance.
     */
    static std::shared_ptr<Arion> ARION_EXPORT
    new_instance(std::vector<std::string> program_args, std::string fs_path = "/",
                 std::vector<std::string> program_env = std::vector<std::string>(), std::string cwd = "",
                 std::unique_ptr<Config> config = std::move(std::make_unique<Config>()));
    /**
     * Initialization method of Arion instances for baremetal programs.
     * @param[in] baremetal A BaremetalManager instance containing parameters to initialize this Arion instance.
     * @param[in] fs_path Path to the root directory for the file system of the emulated process (optional).
     * @param[in] program_env The program environment variables of the emulated process, basically the "envp" array
     * (optional).
     * @param[in] cwd The current working directory for the emulated process (optional).
     * @param[in] config An instance of Config associated with this Arion instance (optional).
     * @return The initialized Arion instance.
     */
    static std::shared_ptr<Arion> ARION_EXPORT
    new_instance(std::unique_ptr<BaremetalManager> baremetal, std::string fs_path = "/",
                 std::vector<std::string> program_env = std::vector<std::string>(), std::string cwd = "",
                 std::unique_ptr<Config> config = std::move(std::make_unique<Config>()));
    /**
     * Destructor method for Arion instances, destroying related instances and engines, and releasing pointers.
     */
    ~Arion();
    /**
     * Used to define start and end addresses for the emulation.
     * @param[in] start Start address for the emulation.
     * @param[in] end End address for the emulation.
     */
    void ARION_EXPORT set_run_bounds(std::optional<ADDR> start = std::nullopt, std::optional<ADDR> end = std::nullopt);
    /**
     * Used to define start address for the emulation.
     * @param[in] start Start address for the emulation.
     */
    void ARION_EXPORT set_run_start(std::optional<ADDR> start);
    /**
     * Used to define end address for the emulation.
     * @param[in] end End address for the emulation.
     */
    void ARION_EXPORT set_run_end(std::optional<ADDR> end);
    /**
     * Starts emulating the process associated with this Arion instance.
     * @return True if the process has not ended yet.
     */
    bool run_current();
    /**
     * Stops emulating the process associated with this Arion instance.
     */
    void ARION_EXPORT stop();
    /**
     * Stops emulating the process associated with this Arion instance with an exception to be rethrown.
     * @param[in] exception The exception to be rethrown.
     */
    void crash(std::exception_ptr exception);
    /**
     * Stops the emulation in order to synchronize the states of its threads. Emulation will be resumed after that
     * synchronization.
     */
    void ARION_EXPORT sync_threads();
    /**
     * Checks whether the process related to this Arion instance is currently being emulated.
     * @return True if the process related to this Arion instance is currently being emulated.
     */
    bool ARION_EXPORT is_running();
    /**
     * Initialize this Arion instance with parameters related to AFL fuzzing.
     * @param[in] signals The list of signals which will be considered as crashes during the fuzzing session.
     */
    void init_afl_mode(std::vector<int> signals);
    /**
     * Clear out paremeters related to AFL fuzzing.
     */
    void stop_afl_mode();
    /**
     * Clone this Arion instance into a new one.
     * @return The new Arion instance.
     */
    std::shared_ptr<Arion> copy();
    /**
     * Checks whether the process emulated by this Arion instance is the child of another instance.
     * @return True if the process emulated by this Arion instance is the child of another instance.
     */
    bool ARION_EXPORT has_parent();
    /**
     * Retrieves the Arion instance associated with the parent of this process.
     * @return The Arion instance associated with the parent of this process.
     */
    std::shared_ptr<Arion> ARION_EXPORT get_parent();
    /**
     * Retrieves the list of Arion instances which this process is a parent of.
     * @return The list of Arion instances which this process is a parent of.
     */
    std::vector<std::shared_ptr<Arion>> ARION_EXPORT get_children();
    /**
     * Retrieves the list of Arion instances in a given PGID group and which this process is a parent of.
     * @param pgid The PGID group.
     * @return The list of Arion instances in a given PGID group and which this process is a parent of.
     */
    std::vector<std::shared_ptr<Arion>> ARION_EXPORT get_pgid_children(pid_t pgid);
    /**
     * Marks a process as a child of the process associated with this Arion instance.
     * @param[in] child The Arion instance which process is to be marked as a child of this process.
     */
    pid_t ARION_EXPORT add_child(std::shared_ptr<Arion> child);
    /**
     * Checks whether a given process is a child of the one associated with this Arion instance.
     * @param[in] child_pid PID of the process to be checked.
     * @return True if the given process is a child of the one associated with this Arion instance.
     */
    bool ARION_EXPORT has_child(pid_t child_pid);
    /**
     * Checks whether a given process is the parent of the one associated with this Arion instance.
     * @param[in] parent_pid PID of the process to be checked.
     * @return True if the given process is the parent of the one associated with this Arion instance.
     */
    bool ARION_EXPORT is_child_of(pid_t parent_pid);
    /**
     * Retrieves a child process of the one associated with this Arion instance.
     * @param[in] child_pid PID of the child process.
     * @return Arion instance associated with the child process.
     */
    std::shared_ptr<Arion> ARION_EXPORT get_child(pid_t child_pid);
    /**
     * Replace the Arion instance associated to a child PID in the "children" list of this instance.
     * @param child_pid The child PID.
     * @param child The Arion instance associated to the child process.
     */
    void ARION_EXPORT replace_child(pid_t child_pid, std::shared_ptr<Arion> child);
    /**
     * Unmarks a process from the "children" list of the process associated with this Arion instance.
     * @param child_pid PID of the process to be unmarked from the "children" list of this process.
     */
    void ARION_EXPORT remove_child(pid_t child_pid);
    /**
     * Removes all processes in the "children" list of this Arion instance.
     */
    void ARION_EXPORT clear_children();
    /**
     * Replaces the image of the process associated with this Arion instance with a new one, like in the execv*
     * syscalls.
     * @param[in] file_path Path to the file of the new process (executable).
     * @param[in] argv The program arguments of the newly emulated process.
     * @param[in] envp The program environment variables of the newly emulated process.
     */
    void execve(std::string file_path, std::vector<std::string> argv, std::vector<std::string> envp);
    /**
     * Retrieves the program arguments associated with this Arion instance.
     * @return The program arguments.
     */
    std::vector<std::string> ARION_EXPORT get_program_args();
    /**
     * Retrieves the program environment variables associated with this Arion instance.
     * @return The program environment variables.
     */
    std::vector<std::string> ARION_EXPORT get_program_env();
    /**
     * Returns the Process ID (PID) associated with this Arion instance.
     * @return The PID.
     */
    pid_t ARION_EXPORT get_pid();
    /**
     * Defines the Process ID (PID) associated with this Arion instance.
     * @param[in] pid The new PID.
     */
    void ARION_EXPORT set_pid(pid_t pid);
    /**
     * Retrieves the Process Group ID (PGID) associated with this Arion instance.
     * @return the PGID.
     */
    pid_t ARION_EXPORT get_pgid();
    /**
     * Defines the Process Group ID (PGID) associated with this Arion instance.
     * @param[in] pgid The new PGID.
     */
    void ARION_EXPORT set_pgid(pid_t pgid);
    /**
     * Retrieves the Session ID (SID) associated with this Arion instance.
     * @return The SID.
     */
    uint32_t ARION_EXPORT get_sid();
    /**
     * Defines the Session ID (SID) associated with this Arion instance.
     * @param[in] sid The new SID.
     */
    void ARION_EXPORT set_sid(uint32_t sid);
    /**
     * Retrieves the User ID (UID) associated with this Arion instance.
     * @return The UID.
     */
    uint32_t ARION_EXPORT get_uid();
    /**
     * Defines the User ID (UID) associated with this Arion instance.
     * @param[in] uid The new UID.
     */
    void ARION_EXPORT set_uid(uint32_t uid);
    /**
     * Retrieves the Group ID (GID) associated with this Arion instance.
     * @return The GID.
     */
    uint32_t ARION_EXPORT get_gid();
    /**
     * Defines the Group ID (GID) associated with this Arion instance.
     * @param[in] gid The new GID.
     */
    void ARION_EXPORT set_gid(uint32_t gid);
    /**
     * Retrieves the Effective User ID (EUID) associated with this Arion instance.
     * @return The EUID.
     */
    uint32_t ARION_EXPORT get_euid();
    /**
     * Defines the Effective User ID (EUID) associated with this Arion instance.
     * @param[in] euid The new EUID.
     */
    void ARION_EXPORT set_euid(uint32_t euid);
    /**
     * Retrieves the Effective Group ID (EGID) associated with this Arion instance.
     * @return The EGID.
     */
    uint32_t ARION_EXPORT get_egid();
    /**
     * Defines the Effective Group ID (EGID) associated with this Arion instance.
     * @param[in] egid The new EGID.
     */
    void ARION_EXPORT set_egid(uint32_t egid);
    /**
     * Checks whether this Arion instance is part of an ArionGroup instance.
     * @return True if this Arion instance is part of an ArionGroup instance.
     */
    bool ARION_EXPORT has_group();
    /**
     * Retrieves the ArionGroup instance associated with this Arion instance.
     * @return The ArionGroup instance associated with this Arion instance.
     */
    std::shared_ptr<ArionGroup> ARION_EXPORT get_group();
    /**
     * Defines the ArionGroup instance associated with this Arion instance.
     * @param[in] group The new ArionGroup for this instance.
     */
    void set_group(std::shared_ptr<ArionGroup> group);
    /**
     * Sets this instance as part of no ArionGroup.
     */
    void reset_group();
    /**
     * Checks whether the emulation is currently stopped.
     * @return True if the emulation is currently stopped.
     */
    bool is_stopped();
    /**
     * Marks the emulation as stopped.
     */
    void set_stopped();
    /**
     * Marks the emulation as running.
     */
    void set_resumed();
    /**
     * Checks whether the emulation has ended for this instance but not for its parent.
     * @return True if the emulation has ended for this instance but not for its parent.
     */
    bool is_zombie();
    /**
     * Marks this process as a zombie (emulation stopped but parent still running).
     */
    void set_zombie();
    /**
     * Delivers a signal to be processed by this process.
     * @param[in] source_pid The Process ID delivering the signal to the process associated with this Arion instance.
     * @param[in] signo The signal number.
     */
    void ARION_EXPORT send_signal(pid_t source_pid, int signo);
    /**
     * Starts a GDB Server session on the given port.
     * @param[in] port The port.
     */
    void ARION_EXPORT run_gdbserver(uint32_t port);
};

}; // namespace arion

#endif // ARION_ARION_HPP
