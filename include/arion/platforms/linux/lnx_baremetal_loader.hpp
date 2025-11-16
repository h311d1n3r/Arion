#ifndef ARION_LNX_BAREMETAL_LOADER_HPP
#define ARION_LNX_BAREMETAL_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_BAREMETAL_CODE_ALIGN 0x1000

#define LINUX_BAREMETAL_CODE_PERMS 0x5

namespace arion
{

/// A Linux loader implementation for baremetal (shellcode) injection/execution without a full ELF file structure.
class LinuxBaremetalLoader : LinuxLoader
{
  private:
    /**
     * Maps the raw machine code (shellcode) into the emulator's memory space.
     * @param[in] code Vector of bytes representing the machine code.
     * @return The base virtual address where the code was mapped.
     */
    ADDR map_code(std::vector<uint8_t> code);

  protected:
    /**
     * Sets up architecture-specific Auxiliary Vector (AUXV) entries before execution starts.
     * This implementation is typically minimal for baremetal loading.
     * @param[in] params Shared pointer to the loader parameters structure.
     * @param[in] arch_sz The size of the architecture (32 or 64 bit).
     */
    void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) override;

  public:
    /**
     * Builder for LinuxBaremetalLoader instances.
     * @param[in] arion Weak pointer to the Arion emulator instance.
     * @param[in] program_args Vector of command-line arguments.
     * @param[in] program_env Vector of environment variables.
     */
    LinuxBaremetalLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_args,
                         const std::vector<std::string> program_env)
        : LinuxLoader(arion, program_args, program_env) {};
    /**
     * Processes the baremetal loading, mapping the code, setting up the stack, and finalizing the loader parameters.
     * @return A unique pointer to the finalized loader parameters (`LNX_LOADER_PARAMS`).
     */
    std::unique_ptr<LNX_LOADER_PARAMS> process() override;
};

}; // namespace arion

#endif // ARION_LNX_BAREMETAL_LOADER_HPP
