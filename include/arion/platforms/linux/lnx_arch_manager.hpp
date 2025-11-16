#ifndef ARION_LNX_ARCH_MANAGER_HPP
#define ARION_LNX_ARCH_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <memory>
#include <vector>

namespace arion
{

/// Structure holding the fully parsed register state for a thread retrieved from a core dump.
struct PARSED_COREDUMP_THREAD
{
    /// Unique pointer to the raw ELF thread data.
    std::unique_ptr<ELF_COREDUMP_THREAD> thread;
    /// Map of Arion universal register IDs to their decoded register values.
    std::map<REG, RVAL> regs;

    /**
     * Builder for PARSED_COREDUMP_THREAD instances.
     */
    PARSED_COREDUMP_THREAD() {};
};

/// Abstract base class for managing Linux architecture-specific details, particularly for core dump parsing.
class LinuxArchManager
{
  private:
    /**
     * **(Pure Virtual)** Converts the raw `prstatus` register data from an ELF core dump into Arion's unified register
     * map.
     * @param[in] prstatus Raw byte vector containing the architecture-specific `prstatus` structure.
     * @return A map of Arion universal register IDs to their values (`arion::RVAL`).
     */
    virtual std::map<REG, RVAL> prstatus_to_regs(std::vector<BYTE> prstatus) = 0;
    /**
     * **(Pure Virtual)** Converts the raw floating-point register data (`fpregset`) from an ELF core dump into Arion's
     * unified register map.
     * @param[in] fpregset Raw byte vector containing the architecture-specific `fpregset` structure.
     * @return A map of Arion universal register IDs to their values (`arion::RVAL`).
     */
    virtual std::map<REG, RVAL> fpregset_to_regs(std::vector<BYTE> fpregset) = 0;

  public:
    /**
     * Parses the raw register data of a single core dump thread using the architecture-specific conversion methods.
     * @param[in] arion Shared pointer to the main Arion instance.
     * @param[in] thread Unique pointer to the raw ELF thread data (`ELF_COREDUMP_THREAD`).
     * @param[in] prog_parser Shared pointer to the ELF parser for context.
     * @return A unique pointer to the fully parsed register data for the thread (`PARSED_COREDUMP_THREAD`).
     */
    std::unique_ptr<PARSED_COREDUMP_THREAD> parse_coredump_thread(std::shared_ptr<Arion> arion,
                                                                  std::unique_ptr<ELF_COREDUMP_THREAD> thread,
                                                                  std::shared_ptr<ElfParser> prog_parser);
};

}; // namespace arion

#endif // ARION_LNX_ARCH_MANAGER_HPP
