#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/platforms/linux/lnx_baremetal_loader.hpp>
#include <cstdint>
#include <memory>

using namespace arion;

std::unique_ptr<LNX_LOADER_PARAMS> LinuxBaremetalLoader::process()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<LNX_LOADER_PARAMS> params = std::make_shared<LNX_LOADER_PARAMS>();
    std::vector<BYTE> code = arion->baremetal->get_code();
    if(code.size())
        params->load_address = this->map_code(code);
    params->stack_address = this->map_stack(params, "Baremetal program");
    this->init_main_thread(params, arion->baremetal->get_entry_addr());
    return std::make_unique<LNX_LOADER_PARAMS>(*params.get());
}

ADDR LinuxBaremetalLoader::map_code(std::vector<BYTE> code) {
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    size_t code_sz = code.size();
    size_t mapping_sz = code_sz;
    if(mapping_sz % LINUX_BAREMETAL_CODE_ALIGN)
        mapping_sz += LINUX_BAREMETAL_CODE_ALIGN - (mapping_sz % LINUX_BAREMETAL_CODE_ALIGN);
    ADDR load_addr = arion->mem->map(arion->baremetal->get_load_addr(), mapping_sz, LINUX_BAREMETAL_CODE_PERMS);
    arion->mem->write(load_addr, code.data(), code.size());
    return load_addr;
}

void LinuxBaremetalLoader::setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) {
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    this->write_auxv_entry(AUXV::AT_ENTRY,arion->baremetal->get_entry_addr());
    this->write_auxv_entry(AUXV::AT_BASE, arion->baremetal->get_load_addr());
}