#ifndef ARION_BAREMETAL_MANAGER_HPP
#define ARION_BAREMETAL_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <memory>
#include <vector>

typedef struct additional_mapped_segments
{
    bool STACK = true;
    bool VDSO = true;
    bool ARM_TRAPS = true;
    bool VVAR = true;
    bool VSYSCALL = true;
} additional_mapped_segments_t;

class ARION_EXPORT BaremetalManager {
private:
    arion::CPU_ARCH arch;
    std::vector<arion::BYTE> code;
    arion::ADDR load_addr;
    arion::ADDR entry_addr;

public:
    ARION_EXPORT BaremetalManager(arion::CPU_ARCH arch, std::vector<arion::BYTE> code, arion::ADDR load_addr, arion::ADDR entry_addr) : arch(arch), code(code), load_addr(load_addr), entry_addr(entry_addr), additional_mapped_segments{} {};
    ARION_EXPORT BaremetalManager(arion::CPU_ARCH arch, std::vector<arion::BYTE> code, arion::ADDR load_addr) : BaremetalManager(arch, code, load_addr, load_addr) {};
    ARION_EXPORT BaremetalManager(arion::CPU_ARCH arch, arion::ADDR load_addr, arion::ADDR entry_addr) : BaremetalManager(arch, std::vector<arion::BYTE>(), load_addr, entry_addr) {};
    arion::CPU_ARCH ARION_EXPORT get_arch();
    std::vector<uint8_t> ARION_EXPORT get_code();
    arion::ADDR ARION_EXPORT get_load_addr();
    arion::ADDR ARION_EXPORT get_entry_addr();

    additional_mapped_segments_t additional_mapped_segments;
};
#endif // ARION_BAREMETAL_MANAGER_HPP
