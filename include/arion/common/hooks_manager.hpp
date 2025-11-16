#ifndef ARION_HOOKS_MANAGER_HPP
#define ARION_HOOKS_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/unicorn/unicorn.h>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <variant>

namespace arion
{

class Arion; // forward declaration to prevent circular dependencies

/// ID associated with a hook when created.
using HOOK_ID = uint64_t;

/**
 * Hook that takes no additional parameter and returns void.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using NO_PARAM_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, void *user_data)>;
/**
 * Hook that takes no additional parameter and returns a boolean.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] user_data Optional user-defined data passed to the hook.
 * @return A boolean, specific to the hook context.
 */
using NO_PARAM_BOOL_HOOK_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, void *user_data)>;
/**
 * Hook that takes a boolean parameter and returns void.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] val A uint32_t value, specific to the hook context.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using U32_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uint32_t val, void *user_data)>;
/**
 * Hook that takes address and size parameters and returns void.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] addr A memory address, specific to the hook context.
 * @param[in] sz A memory size, specific to the hook context.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using ADDR_SZ_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)>;
/**
 * Hook associated with a memory operation.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] type Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr The memory address being accessed.
 * @param[in] size Size of the memory access.
 * @param[in] val Value being read or written (if applicable).
 * @param[in] user_data Optional user-defined data passed to the hook.
 * @return True if the hook handled the event and should suppress default behavior; false otherwise.
 */
using MEM_HOOK_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, uc_mem_type type, uint64_t addr, int size,
                                             int64_t val, void *user_data)>;
/**
 * Hook that is called when execution transitions between two translation blocks.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] cur Current translation block.
 * @param[in] prev Previous translation block.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using EDGE_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uc_tb *cur, uc_tb *prev, void *user_data)>;
/**
 * Hook for TCG-level emulation events.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] addr Guest address where the event occurred.
 * @param[in] arg1 First argument associated with the event.
 * @param[in] arg2 Second argument associated with the event.
 * @param[in] size Size (in bytes) of the operation or data.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using TCG_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uint64_t addr, uint64_t arg1, uint64_t arg2,
                                             int size, void *user_data)>;
/**
 * Hook that intercepts TLB (Translation Lookaside Buffer) resolution events.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] addr Guest virtual address being translated.
 * @param[in] type Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in, out] result Resulting TLB entry.
 * @param[in] user_data Optional user-defined data passed to the hook.
 * @return True if the hook provided a custom translation; false to continue normal translation.
 */
using TLB_HOOK_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, uint64_t addr, uc_mem_type type,
                                             uc_tlb_entry *result, void *user_data)>;
/**
 * Hook that is triggered when a new process is created (e.g., via fork or clone).
 * @param[in] arion Parent Arion instance that triggered the hook.
 * @param[in] child Child Arion instance that got created.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using PROCESS_HOOK_CALLBACK =
    std::function<void(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> child, void *user_data)>;
/**
 * Hook that is triggered on syscall invocation.
 * @param[in] arion Arion instance that triggered the hook.
 * @param[in] sysno System call number.
 * @param[in] params Vector of system call parameters.
 * @param[out] handled Output flag; set to true if the hook fully handled the syscall and default handling should be
 * skipped.
 * @param[in] user_data Optional user-defined data passed to the hook.
 */
using SYSCALL_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uint64_t sysno,
                                                 std::vector<SYS_PARAM> params, bool *handled, void *user_data)>;
/// Variant type that represents any possible hook callback type supported by Arion.
using HOOK_CALLBACK = std::variant<NO_PARAM_HOOK_CALLBACK, NO_PARAM_BOOL_HOOK_CALLBACK, U32_HOOK_CALLBACK,
                                   ADDR_SZ_HOOK_CALLBACK, MEM_HOOK_CALLBACK, EDGE_HOOK_CALLBACK, TCG_HOOK_CALLBACK,
                                   TLB_HOOK_CALLBACK, PROCESS_HOOK_CALLBACK, SYSCALL_HOOK_CALLBACK>;

/// Types for Arion hooks.
enum ARION_HOOK_TYPE
{
    INTR_HOOK,               ///< Triggered when an interrupt occurs (e.g., software or hardware interrupt).
    INSN_HOOK,               ///< Triggered before or after executing a specific instruction.
    CODE_HOOK,               ///< Triggered when code execution reaches a specified address or range.
    BLOCK_HOOK,              ///< Triggered at the start of a new basic block or translation block (TB).
    MEM_READ_UNMAPPED_HOOK,  ///< Triggered on a memory read from an unmapped region.
    MEM_WRITE_UNMAPPED_HOOK, ///< Triggered on a memory write to an unmapped region.
    MEM_FETCH_UNMAPPED_HOOK, ///< Triggered when fetching instructions from an unmapped region.
    MEM_READ_PROT_HOOK,      ///< Triggered on a memory read violating protection flags (e.g., non-readable region).
    MEM_WRITE_PROT_HOOK,     ///< Triggered on a memory write violating protection flags (e.g., read-only region).
    MEM_FETCH_PROT_HOOK,     ///< Triggered when fetching instructions from a non-executable or protected region.
    MEM_READ_HOOK,           ///< Triggered on every valid memory read access.
    MEM_WRITE_HOOK,          ///< Triggered on every valid memory write access.
    MEM_FETCH_HOOK,          ///< Triggered when fetching instructions from valid mapped memory.
    MEM_READ_AFTER_HOOK,     ///< Triggered after a memory read operation completes (e.g., for tracing or logging).
    INSN_INVALID_HOOK,       ///< Triggered when an invalid or unimplemented instruction is encountered.
    EDGE_GENERATED_HOOK,     ///< Triggered when a new control-flow edge (TB → TB) is generated during emulation.
    TCG_OPCODE_HOOK,         ///< Triggered for each TCG opcode during translation (for instrumentation or analysis).
    TLB_FILL_HOOK,           ///< Triggered when the TLB is filled or a translation lookup occurs.
    FORK_HOOK,               ///< Triggered when a process forks or clones (child Arion instance is created).
    EXECVE_HOOK,             ///< Triggered when a process performs an execve-like operation (program replacement).
    SYSCALL_HOOK             ///< Triggered on system call invocation (before or after handling).
};

/// A map identifying a Unicorn hook type given its associated Arion hook type.
extern std::map<ARION_HOOK_TYPE, uc_hook_type> ARION_UC_HOOK_TYPES;
/// A map identifying a hook function given its associated Arion hook type.
extern std::map<ARION_HOOK_TYPE, void *> ARION_UC_HOOK_FUNCS;

/// This structure is placed in the Unicorn user_data parameter of hooks.
struct ARION_HOOK_PARAM
{
    /// Arion instance that triggered the hook.
    std::weak_ptr<Arion> arion;
    /// A user-defined callback for the hook.
    HOOK_CALLBACK callback;
    /// Optional user-defined data passed to the hook.
    void *user_data;
    /**
     * Builder for ARION_HOOK_PARAM instances.
     * @param[in] arion Arion instance that triggered the hook.
     * @param[in] callback A user-defined callback for the hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     */
    ARION_HOOK_PARAM(std::weak_ptr<Arion> arion, HOOK_CALLBACK callback, void *user_data)
        : arion(arion), callback(callback), user_data(user_data) {};
};

/// This structure holds information about an Arion hook.
struct ARION_HOOK
{
    /// Arion type for the hook.
    ARION_HOOK_TYPE type;
    /// Unicorn hook id associated with this Arion hook.
    uc_hook uc_id;
    /// A structure holding data to be passed to the Arion hook when its associated Unicorn hook gets triggered.
    ARION_HOOK_PARAM *param;
    /**
     * Builder for ARION_HOOK instances.
     * @param[in] type Arion type for the hook.
     * @param[in] uc_id Unicorn hook id associated with this Arion hook.
     * @param[in] param A structure holding data to be passed to the Arion hook when its associated Unicorn hook gets
     * triggered.
     */
    ARION_HOOK(ARION_HOOK_TYPE type, uc_hook uc_id, ARION_HOOK_PARAM *param)
        : type(type), uc_id(uc_id), param(param) {};
};

/**
 * Unicorn hook that gets triggered when an interrupt occurs (e.g., software or hardware interrupt).
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] intno Interrupt number.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 */
void arion_intr_hook(uc_engine *uc, uint32_t intno, void *user_data);
/**
 * Unicorn hook that gets triggered before or after executing a specific instruction.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 */
void arion_insn_hook(uc_engine *uc, void *user_data);
/**
 * Unicorn hook that gets triggered when code execution reaches a specified address or range.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] address Memory address of the instruction being executed.
 * @param[in] size Size of the executed instruction or block in bytes.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 */
void arion_code_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
/**
 * Unicorn hook that gets triggered at the start of a new basic block or translation block (TB).
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] address Start memory address of the block.
 * @param[in] size Size of the translation block in bytes.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 */
void arion_block_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
/**
 * Unicorn hook that gets triggered on a memory read from an unmapped region.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being accessed.
 * @param[in] size Size of the memory access.
 * @param[in] val Value attempted to be read.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_read_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                  void *user_data);
/**
 * Unicorn hook that gets triggered on a memory write to an unmapped region.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being accessed.
 * @param[in] size Size of the memory access.
 * @param[in] val Value attempted to be written.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_write_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                   void *user_data);
/**
 * Unicorn hook that gets triggered when fetching instructions from an unmapped region.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being fetched from.
 * @param[in] size Size of the fetch operation.
 * @param[in] val Ignored for fetch operations.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_fetch_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                   void *user_data);
/**
 * Unicorn hook that gets triggered on a memory read violating protection flags.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being accessed.
 * @param[in] size Size of the memory access.
 * @param[in] val Value attempted to be read.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_read_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
/**
 * Unicorn hook that gets triggered on a memory write violating protection flags.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being accessed.
 * @param[in] size Size of the memory access.
 * @param[in] val Value attempted to be written.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_write_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                               void *user_data);
/**
 * Unicorn hook that gets triggered when fetching instructions from a protected region.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being fetched from.
 * @param[in] size Size of the fetch operation.
 * @param[in] val Ignored for fetch operations.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_fetch_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                               void *user_data);
/**
 * Unicorn hook that gets triggered on every valid memory read access.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being read.
 * @param[in] size Size of the memory read.
 * @param[in] val Value read from memory.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_read_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
/**
 * Unicorn hook that gets triggered on every valid memory write access.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being written.
 * @param[in] size Size of the memory write.
 * @param[in] val Value written to memory.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_write_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);

/**
 * Unicorn hook that gets triggered when fetching instructions from valid mapped memory.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address being fetched from.
 * @param[in] size Size of the fetch operation.
 * @param[in] val Ignored for fetch operations.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_fetch_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
/**
 * Unicorn hook that gets triggered after a memory read operation completes.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in] addr Memory address that was read.
 * @param[in] size Size of the read operation.
 * @param[in] val Value read from memory.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the access and default behavior should be suppressed.
 */
bool arion_mem_read_after_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                               void *user_data);
/**
 * Unicorn hook that gets triggered when an invalid or unimplemented instruction is encountered.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the invalid instruction and default behavior should be suppressed.
 */
bool arion_insn_invalid_hook(uc_engine *uc, void *user_data);
/**
 * Unicorn hook that gets triggered when a new control-flow edge (translation block to translation block) is generated.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] cur Current translation block.
 * @param[in] prev Previous translation block.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 */
void arion_edge_generated_hook(uc_engine *uc, uc_tb *cur, uc_tb *prev, void *user_data);
/**
 * Unicorn hook that gets triggered for each TCG opcode during translation or emulation.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] addr Guest address of the instruction or opcode.
 * @param[in] arg1 First argument associated with the opcode.
 * @param[in] arg2 Second argument associated with the opcode.
 * @param[in] size Size of the operation or data in bytes.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 */
void arion_tcg_opcode_hook(uc_engine *uc, uint64_t addr, uint64_t arg1, uint64_t arg2, int size, void *user_data);
/**
 * Unicorn hook that gets triggered when a TLB entry is filled or a translation lookup occurs.
 * @param[in] uc The unicorn engine that triggered the hook.
 * @param[in] addr Guest virtual address being translated.
 * @param[in] access Type of memory access (e.g., UC_MEM_READ, UC_MEM_WRITE, UC_MEM_FETCH).
 * @param[in, out] result Resulting TLB entry, which can be modified by the hook.
 * @param[in] user_data A ARION_HOOK_PARAM structure which holds data related to the associated Arion hook.
 * @return True if the hook handled the translation and default behavior should be suppressed.
 */
bool arion_tlb_fill_hook(uc_engine *uc, uint64_t addr, uc_mem_type type, uc_tlb_entry *result, void *user_data);

/// This class purpose is to manage user-defined Arion hooks including hook creation, deletion, trigger handling...
class ARION_EXPORT HooksManager
{
  private:
    /// The Arion instance associated with this instance.
    std::weak_ptr<Arion> arion;
    /// The Unicorn engine associated with this instance.
    uc_engine *uc;
    /// ID of the next hook to be created.
    HOOK_ID curr_id = 1;
    /// A map identifying an Arion hook given its ID.
    std::map<HOOK_ID, std::shared_ptr<ARION_HOOK>> hooks;
    /// A stack of hook IDs that got deleted and that can be reused.
    std::stack<HOOK_ID> free_hook_ids;
    /**
     * Generates a new hook ID.
     * @return The new ID.
     */
    HOOK_ID gen_next_id();
    /**
     * Creates a new hook related to the Unicorn engine, which means every hooks that are defined in Unicorn.
     * @tparam UcParams Additional parameters for genericity purpose.
     * @param[in] type Arion type for the hook.
     * @param[in] callback Arion callback which gets called when the Unicorn hook is triggered.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @param[in] start Start memory address where the hook should trigger.
     * @param[in] end End memory address where the hook should trigger.
     * @param[in] uc_params Additional parameters for the hook.
     * @return The new hook ID.
     */
    template <typename... UcParams>
    HOOK_ID hook_uc(ARION_HOOK_TYPE type, HOOK_CALLBACK callback, void *user_data, ADDR start = 0,
                    ADDR end = ARION_MAX_U64, UcParams... uc_params);
    /**
     * Creates a new hook which is not related to the Unicorn engine.
     * @param[in] type Arion type for the hook.
     * @param[in] callback Arion callback which gets called when the Unicorn hook is triggered.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID hook_arion(ARION_HOOK_TYPE type, HOOK_CALLBACK callback, void *user_data);

  public:
    /**
     * Builder for HooksManager instances.
     * @param[in] arion The Arion instance associated with this instance.
     */
    HooksManager(std::weak_ptr<Arion> arion);
    /**
     * Destructor for HooksManager instances.
     */
    ~HooksManager();
    /**
     * Instanciates and initializes new HooksManager objects with some parameters.
     * @param[in] arion The Arion instance associated with this instance.
     * @return A new HooksManager instance.
     */
    static std::unique_ptr<HooksManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Creates a new hook that gets triggered when an interrupt occurs (e.g., software or hardware interrupt).
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address where the hook should trigger.
     * @param[in] end End memory address where the hook should trigger.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_intr(U32_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                   void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered before or after executing a specific instruction.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] insn Instruction identifier (architecture-specific).
     * @param[in] start Start memory address where the hook should trigger.
     * @param[in] end End memory address where the hook should trigger.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_insn(NO_PARAM_HOOK_CALLBACK callback, uint64_t insn, ADDR start = 0,
                                   ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when code execution reaches a specified address or range.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address where the hook should trigger.
     * @param[in] end End memory address where the hook should trigger.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_code(ADDR_SZ_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                   void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when execution reaches a specific address.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] addr Exact memory address where the hook should trigger.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_addr(ADDR_SZ_HOOK_CALLBACK callback, ADDR addr, void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered at the start of a new basic block or translation block (TB).
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address where the hook should trigger.
     * @param[in] end End memory address where the hook should trigger.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_block(ADDR_SZ_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                    void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered on a memory read from an unmapped region.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_read_unmapped(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                                void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered on a memory write to an unmapped region.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_write_unmapped(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                                 void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when fetching instructions from an unmapped region.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_fetch_unmapped(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                                 void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered on a memory read violating protection flags.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_read_prot(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                            void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered on a memory write violating protection flags.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_write_prot(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                             void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when fetching instructions from a protected region.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_fetch_prot(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                             void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered on every valid memory read access.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_read(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                       void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered on every valid memory write access.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_write(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                        void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when fetching instructions from valid mapped memory.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_fetch(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                        void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered after a memory read operation completes.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_mem_read_after(MEM_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                             void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when an invalid or unimplemented instruction is encountered.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_insn_invalid(NO_PARAM_BOOL_HOOK_CALLBACK callback, ADDR start = 0,
                                           ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when a new control-flow edge (translation block to translation block) is
     * generated.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_edge_generated(EDGE_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                             void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered for each TCG opcode during translation or emulation.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] aux1 Auxiliary value (architecture-specific argument 1).
     * @param[in] aux2 Auxiliary value (architecture-specific argument 2).
     * @param[in] start Start memory address where the hook should trigger.
     * @param[in] end End memory address where the hook should trigger.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_tcg_opcode(TCG_HOOK_CALLBACK callback, uint64_t aux1, uint64_t aux2, ADDR start = 0,
                                         ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when a TLB entry is filled or a translation lookup occurs.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] start Start memory address range monitored by this hook.
     * @param[in] end End memory address range monitored by this hook.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_tlb_fill(TLB_HOOK_CALLBACK callback, ADDR start = 0, ADDR end = ARION_MAX_U64,
                                       void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when a process forks (via fork(), vfork(), or clone()).
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_fork(PROCESS_HOOK_CALLBACK callback, void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when a process executes a new program (via execve()).
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_execve(PROCESS_HOOK_CALLBACK callback, void *user_data = nullptr);
    /**
     * Creates a new hook that gets triggered when a system call is invoked.
     * @param[in] callback Callback being called when this hook gets triggered.
     * @param[in] user_data Optional user-defined data passed to the hook.
     * @return The new hook ID.
     */
    HOOK_ID ARION_EXPORT hook_syscall(SYSCALL_HOOK_CALLBACK callback, void *user_data = nullptr);
    /**
     * Deletes an Arion hook, preventing it from being triggered.
     * @param[in] hook_id ID of the hook which must get deleted.
     */
    void ARION_EXPORT unhook(HOOK_ID hook_id);
    /**
     * Deletes all Arion hooks, preventing them from being triggered.
     */
    void ARION_EXPORT clear_hooks();

    /**
     * Triggers all hooks with a given type, which are not related to the Unicorn engine.
     * @tparam HookParams Additional parameters for genericity purpose.
     * @param[in] type Arion type for the hooks to be triggered.
     * @param[in] params Additional parameters for the hooks.
     */
    template <typename... HookParams> void trigger_arion_hook(ARION_HOOK_TYPE type, HookParams... params)
    {
        for (auto &hook : this->hooks)
        {
            if (hook.second->type == type)
            {
                HOOK_CALLBACK &callback = hook.second->param->callback;
                std::shared_ptr<Arion> arion = hook.second->param->arion.lock();
                if (!arion)
                    throw arion_exception::ExpiredWeakPtrException("Arion");
                void *user_data = hook.second->param->user_data;

                std::visit(
                    [&](auto &&cb) {
                        using CallbackType = std::decay_t<decltype(cb)>;
                        if constexpr (std::is_invocable_v<CallbackType, std::shared_ptr<Arion>, HookParams..., void *>)
                            cb(arion, params..., user_data);
                        else
                            throw arion_exception::WrongHookParamsException();
                    },
                    callback);
            }
        }
    }
};

}; // namespace arion

#endif // ARION_HOOKS_MANAGER_HPP
