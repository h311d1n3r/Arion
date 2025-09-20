#ifndef ARION_BAREMETAL_MANAGER_HPP
#define ARION_BAREMETAL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <memory>
#include <vector>

namespace arion
{

class ARION_EXPORT BaremetalManager
{
  private:
    CPU_ARCH arch;
    std::vector<BYTE> code;
    ADDR load_addr;
    ADDR entry_addr;

  public:
    ARION_EXPORT BaremetalManager(CPU_ARCH arch, std::vector<BYTE> code, ADDR load_addr, ADDR entry_addr)
        : arch(arch), code(code), load_addr(load_addr), entry_addr(entry_addr) {};
    ARION_EXPORT BaremetalManager(CPU_ARCH arch, std::vector<BYTE> code, ADDR load_addr)
        : BaremetalManager(arch, code, load_addr, load_addr) {};
    ARION_EXPORT BaremetalManager(CPU_ARCH arch, ADDR load_addr, ADDR entry_addr)
        : BaremetalManager(arch, std::vector<BYTE>(), load_addr, entry_addr) {};
    CPU_ARCH ARION_EXPORT get_arch();
    std::vector<uint8_t> ARION_EXPORT get_code();
    ADDR ARION_EXPORT get_load_addr();
    ADDR ARION_EXPORT get_entry_addr();
};

}; // namespace arion

#endif // ARION_BAREMETAL_MANAGER_HPP
