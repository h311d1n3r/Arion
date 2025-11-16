#ifndef ARION_ELF_LOADER_HPP
#define ARION_ELF_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_loader.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_64_VVAR_ADDR 0x7ffff7fba000
#define LINUX_64_VVAR_SZ 0x4000
#define LINUX_64_VDSO_ADDR 0x7ffff7fbe000
#define LINUX_64_VDSO_SZ 0x2000
#define LINUX_64_INTERP_ADDR 0x7ffff7fc0000
#define LINUX_64_VSYSCALL_ADDR 0xffffffffff600000
#define LINUX_64_VSYSCALL_ALIGN 0x1000

#define LINUX_32_VVAR_ADDR 0xf7fba000
#define LINUX_32_VVAR_SZ 0x4000
#define LINUX_32_VDSO_ADDR 0xf7fbe000
#define LINUX_32_VDSO_SZ 0x2000
#define LINUX_32_INTERP_ADDR 0xf7fc0000
#define LINUX_32_ARM_TRAPS_ADDR 0xffff0000
#define LINUX_32_ARM_TRAPS_SIZE 0x1000
#define LINUX_32_ARM_GETTLS_ADDR 0xffff0fe0

#define LINUX_VVAR_PERMS 4
#define LINUX_VDSO_PERMS 5
#define LINUX_VSYSCALL_PERMS 1
#define LINUX_ARM_TRAPS_PERMS 5

#define HEAP_SZ 0x1000
#define HEAP_PERMS 6

#define LINUX_VDSO_KERNEL_VSYSCALL_OFF 0x570

/// External symbol indicating the start of the embedded VDSO binary data.
extern char _binary_vdso_bin_start[];
/// External symbol indicating the end of the embedded VDSO binary data.
extern char _binary_vdso_bin_end[];

namespace arion
{

/// Structure representing a kernel trap entry for the 32-bit ARM architecture.
struct ARM_TRAP
{
    /// Name of the trap function (e.g., syscall).
    std::string name;
    /// Offset within the ARM traps page.
    off_t off;
    /// The actual machine code bytes of the trap.
    std::vector<BYTE> code;

    /**
     * Builder for ARM_TRAP instances.
     * @param[in] name Name of the trap function.
     * @param[in] off Offset within the traps page.
     * @param[in] code The code bytes.
     */
    ARM_TRAP(std::string name, off_t off, std::vector<BYTE> code)
        : name(std::move(name)), off(off), code(std::move(code)) {};
};

/// A Linux loader implementation responsible for parsing and mapping ELF binaries (executables and dynamic loaders).
class ElfLoader : LinuxLoader
{
  private:
    /// ELF parser for the program interpreter (dynamic linker).
    std::shared_ptr<ElfParser> prog_parser, interp_parser;
    /// Flag indicating if the program is Position-Independent Executable (PIE).
    bool is_pie;
    /// Flag indicating if the program is statically linked.
    bool is_static;
    /**
     * Maps the loadable segments of an ELF binary into the emulator's memory space.
     * @param[in] parser The ELF parser instance.
     * @param[in] load_addr The base virtual address to load the segments at.
     * @return The actual base address where the ELF segments were loaded.
     */
    ADDR map_elf_segments(const std::shared_ptr<ElfParser> parser, ADDR load_addr);
    /**
     * Maps the kernel's Virtual Variable (VVAR) page into memory.
     * @return The base virtual address of the VVAR page.
     */
    ADDR map_vvar();
    /**
     * Maps the kernel's Virtual Dynamic Shared Object (VDSO) page into memory.
     * @return The base virtual address of the VDSO page.
     */
    ADDR map_vdso();
    /**
     * Maps the kernel's Virtual Syscall (VSYSCALL) page into memory.
     * @return The base virtual address of the VSYSCALL page.
     */
    ADDR map_vsyscall();
    /**
     * Maps the 32-bit ARM kernel trap page into memory.
     * @return The base virtual address of the ARM traps page.
     */
    ADDR map_arm_traps();
    /**
     * Initializes thread synchronization structures related to coredumps.
     */
    void init_coredump_threads();

  protected:
    /**
     * Sets up architecture-specific Auxiliary Vector (AUXV) entries before execution starts.
     * @param[in] params Shared pointer to the loader parameters structure.
     * @param[in] arch_sz The size of the architecture (32 or 64 bit).
     */
    void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) override;

  public:
    /**
     * Builder for a dynamically linked ElfLoader instance.
     * @param[in] arion Weak pointer to the Arion emulator instance.
     * @param[in] interp_parser ELF parser for the dynamic linker.
     * @param[in] prog_parser ELF parser for the main program.
     * @param[in] program_args Vector of command-line arguments.
     * @param[in] program_env Vector of environment variables.
     */
    ElfLoader(std::weak_ptr<Arion> arion, std::shared_ptr<ElfParser> interp_parser,
              std::shared_ptr<ElfParser> prog_parser, const std::vector<std::string> program_args,
              const std::vector<std::string> program_env)
        : LinuxLoader(arion, program_args, program_env), interp_parser(interp_parser), prog_parser(prog_parser) {};
    /**
     * Builder for a statically linked ElfLoader instance.
     * @param[in] arion Weak pointer to the Arion emulator instance.
     * @param[in] prog_parser ELF parser for the main program.
     * @param[in] program_args Vector of command-line arguments.
     * @param[in] program_env Vector of environment variables.
     */
    ElfLoader(std::weak_ptr<Arion> arion, std::shared_ptr<ElfParser> prog_parser,
              const std::vector<std::string> program_args, const std::vector<std::string> program_env)
        : ElfLoader(arion, nullptr, std::move(prog_parser), program_args, program_env) {};
    /**
     * Processes the ELF binary loading, mapping segments, setting up the stack, and finalizing the loader parameters.
     * @return A unique pointer to the finalized loader parameters (`LNX_LOADER_PARAMS`).
     */
    std::unique_ptr<LNX_LOADER_PARAMS> process() override;
};

}; // namespace arion

#endif // ARION_ELF_LOADER_HPP
