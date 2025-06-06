#ifndef ARION_BAREMETAL_HPP
#define ARION_BAREMETAL_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <memory>
#include <vector>

struct ARION_EXPORT Baremetal {
    std::shared_ptr<std::vector<uint8_t>> coderaw = std::make_shared<std::vector<uint8_t>>();
    uint16_t bitsize = 64;
    arion::CPU_ARCH arch = arion::CPU_ARCH::X8664_ARCH;
    bool setup_memory = false;
};

void ARION_EXPORT init_default_instance(std::shared_ptr<Arion> arion);

#endif // ARION_BAREMETAL_HPP
