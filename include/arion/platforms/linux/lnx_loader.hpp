#ifndef ARION_LNX_LOADER_HPP
#define ARION_LNX_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_64_LOAD_ADDR 0x400000
#define LINUX_64_STACK_ADDR 0x7ffffffde000
#define LINUX_64_STACK_SZ 0x21000

#define LINUX_32_LOAD_ADDR 0x8040000
#define LINUX_32_STACK_ADDR 0xfffcf000
#define LINUX_32_STACK_SZ 0x21000

#define LINUX_STACK_PERMS 6

namespace arion
{

/// Enumeration of Auxiliary Vector (AUXV) keys passed by the kernel to the user space.
enum AUXV
{
    /// End of the auxiliary vector.
    AT_NULL = 0,
    /// Entry ignored.
    AT_IGNORE = 1,
    /// File descriptor of the executable.
    AT_EXECFD = 2,
    /// Program header table address.
    AT_PHDR = 3,
    /// Size of program header table entry.
    AT_PHENT = 4,
    /// Number of program header entries.
    AT_PHNUM = 5,
    /// System page size.
    AT_PAGESZ = 6,
    /// Base address of the interpreter/dynamic linker.
    AT_BASE = 7,
    /// Flags for the executable.
    AT_FLAGS = 8,
    /// Entry point of the executable.
    AT_ENTRY = 9,
    /// Value is non-zero if the executable is not an ELF file.
    AT_NOTELF = 10,
    /// Real User ID.
    AT_UID = 11,
    /// Effective User ID.
    AT_EUID = 12,
    /// Real Group ID.
    AT_GID = 13,
    /// Effective Group ID.
    AT_EGID = 14,
    /// String identifying the kernel platform (e.g., "x86_64").
    AT_PLATFORM = 15,
    /// Bitmask of CPU hardware capabilities.
    AT_HWCAP = 16,
    /// Clock tick frequency (jiffies).
    AT_CLKTCK = 17,
    /// Non-zero if the process is being run with a security taint.
    AT_SECURE = 23,
    /// String identifying the base platform.
    AT_BASE_PLATFORM = 24,
    /// Address of 16 bytes of random data.
    AT_RANDOM = 25,
    /// Additional bitmask of CPU hardware capabilities.
    AT_HWCAP2 = 26,
    /// Size of the rseq structure.
    AT_RSEQ_FEATURE_SIZE = 27,
    /// Alignment of the rseq structure.
    AT_RSEQ_ALIGN = 28,
    /// Third bitmask of CPU hardware capabilities.
    AT_HWCAP3 = 29,
    /// Fourth bitmask of CPU hardware capabilities.
    AT_HWCAP4 = 30,
    /// Address of the executable's pathname string.
    AT_EXECFN = 31,
    /// Address of a function to call for vsyscall/vdso.
    AT_SYSINFO = 32,
    /// Address of the VDSO/VSYSCALL ELF header.
    AT_SYSINFO_EHDR = 33,
    /// Minimum stack size for a signal handler.
    AT_MINSIGSTKSZ = 51
};

/// Structure holding allocated memory addresses for string data placed on the stack for AUXV.
struct AUXV_PTRS
{
    /// Address of the random data (AT_RANDOM target).
    ADDR random_addr;
    /// Address of the program path string (AT_EXECFN target).
    ADDR prog_name_addr;
    /// Address of the platform string (AT_PLATFORM target).
    ADDR platform_name_addr;
};

/// Structure holding the key load addresses of the emulated process.
struct ARION_EXPORT LNX_LOADER_PARAMS
{
    /// Base address where the main executable was loaded.
    ADDR load_address;
    /// Base address where the dynamic interpreter was loaded.
    ADDR interp_address;
    /// Base address where the VVAR page was mapped.
    ADDR vvar_address;
    /// Base address where the VDSO page was mapped.
    ADDR vdso_address;
    /// Base address of the program stack.
    ADDR stack_address;
    /// Base address of the VSYSCALL page.
    ADDR vsyscall_address;
    /// Base address of the ARM traps page (32-bit ARM specific).
    ADDR arm_traps_address;
};

/// Abstract base class for Linux-specific executable loading and setup routines.
class LinuxLoader
{
  private:
    /**
     * Completes the setup of the Auxiliary Vector, writing key addresses and values to the stack.
     * @param[in] auxv_ptrs Unique pointer to the structure holding temporary addresses for string placement.
     * @param[in] params Shared pointer to the structure holding loaded addresses.
     */
    void setup_auxv(std::unique_ptr<AUXV_PTRS> auxv_ptrs, std::shared_ptr<LNX_LOADER_PARAMS> params);
    /**
     * Writes the environment variable pointers to the stack.
     * @param[in] envp_ptrs Vector of memory addresses pointing to the environment strings.
     */
    void setup_envp(std::vector<ADDR> envp_ptrs);
    /**
     * Writes the argument pointers (`argv`) to the stack.
     * @param[in] argv_ptrs Vector of memory addresses pointing to the argument strings.
     */
    void setup_argv(std::vector<ADDR> argv_ptrs);

  protected:
    /// Weak pointer back to the main Arion instance.
    std::weak_ptr<Arion> arion;
    /// Vector of command-line arguments.
    const std::vector<std::string> program_args;
    /// Vector of environment variables.
    const std::vector<std::string> program_env;
    /**
     * Builder for LinuxLoader instances.
     * @param[in] arion Weak pointer to the Arion emulator instance.
     * @param[in] program_args Vector of command-line arguments.
     * @param[in] program_env Vector of environment variables.
     */
    LinuxLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_args,
                const std::vector<std::string> program_env)
        : arion(arion), program_args(program_args), program_env(program_env) {};
    /**
     * Writes a single entry (key-value pair) to the Auxiliary Vector on the stack.
     * @param[in] auxv The `AUXV` key.
     * @param[in] val The corresponding value to store.
     */
    void write_auxv_entry(AUXV auxv, uint64_t val);
    /**
     * **(Pure Virtual)** Sets up architecture-specific AUXV entries (to be implemented by derived classes).
     * @param[in] params Shared pointer to the loader parameters structure.
     * @param[in] arch_sz The size of the architecture (32 or 64 bit).
     */
    virtual void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) = 0;
    /**
     * Maps the initial process stack into memory and writes the argument/environment strings onto it.
     * @param[in] params Shared pointer to the loader parameters structure.
     * @param[in] path The path to the executable file.
     * @return The base address of the mapped stack.
     */
    ADDR map_stack(std::shared_ptr<LNX_LOADER_PARAMS> params, std::string path);
    /**
     * Initializes the main thread's registers (e.g., PC, stack pointer) for execution to begin.
     * @param[in] params Shared pointer to the loader parameters structure.
     * @param[in] entry_addr The initial entry point address (usually the program or interpreter entry point).
     */
    void init_main_thread(std::shared_ptr<LNX_LOADER_PARAMS> params, ADDR entry_addr);

  public:
    /**
     * **(Pure Virtual)** Main loading routine for the process (to be implemented by derived classes).
     * @return A unique pointer to the finalized loader parameters (`LNX_LOADER_PARAMS`).
     */
    virtual std::unique_ptr<LNX_LOADER_PARAMS> process() = 0;
};

}; // namespace arion

#endif // ARION_LNX_LOADER_HPP
