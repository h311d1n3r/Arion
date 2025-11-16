#ifndef ARION_BAREMETAL_MANAGER_HPP
#define ARION_BAREMETAL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <vector>

namespace arion
{

/// This class is responsible for configuring a session of baremetal emulation. After being configured, a
/// BaremetalManager instance can be passed to a specific Arion initialization method.
class ARION_EXPORT BaremetalManager
{
  private:
    /// Arion CPU architecture for emulation.
    CPU_ARCH arch;
    /// The code to be emulated.
    std::vector<BYTE> code;
    /// The memory address at which to map the code.
    ADDR load_addr;
    /// The memory address at which to start emulation.
    ADDR entry_addr;

  public:
    /**
     * Builder method for BaremetalManager instances.
     * @param[in] arch Arion CPU architecture for emulation.
     * @param[in] code The code to be emulated.
     * @param[in] load_addr The memory address at which to map the code.
     * @param[in] entry_addr The memory address at which to start emulation.
     */
    ARION_EXPORT BaremetalManager(CPU_ARCH arch, std::vector<BYTE> code, ADDR load_addr, ADDR entry_addr)
        : arch(arch), code(code), load_addr(load_addr), entry_addr(entry_addr) {};
    /**
     * Builder method for BaremetalManager instances.
     * @param[in] arch Arion CPU architecture for emulation.
     * @param[in] code The code to be emulated.
     * @param[in] load_addr The memory address at which to map the code.
     */
    ARION_EXPORT BaremetalManager(CPU_ARCH arch, std::vector<BYTE> code, ADDR load_addr)
        : BaremetalManager(arch, code, load_addr, load_addr) {};
    /**
     * Builder method for BaremetalManager instances.
     * @param[in] arch Arion CPU architecture for emulation.
     * @param[in] load_addr The memory address at which to map the code.
     * @param[in] entry_addr The memory address at which to start emulation.
     */
    ARION_EXPORT BaremetalManager(CPU_ARCH arch, ADDR load_addr, ADDR entry_addr)
        : BaremetalManager(arch, std::vector<BYTE>(), load_addr, entry_addr) {};
    /**
     * Retrieves the Arion CPU architecture associated with this instance.
     * @return The Arion CPU architecture.
     */
    CPU_ARCH ARION_EXPORT get_arch();
    /**
     * Retrieves the assembled instructions of code to be emulated in baremetal.
     * @return The vector of assembled instructions data.
     */
    std::vector<uint8_t> ARION_EXPORT get_code();
    /**
     * Retrieves the memory address at which the code is to be loaded.
     * @return The code starting memory address.
     */
    ADDR ARION_EXPORT get_load_addr();
    /**
     * Retrieves the memory address at which the code should start being emulated.
     * @return The code entry address.
     */
    ADDR ARION_EXPORT get_entry_addr();
};

}; // namespace arion

#endif // ARION_BAREMETAL_MANAGER_HPP
