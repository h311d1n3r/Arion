#include <arion/common/baremetal_manager.hpp>

using namespace arion;

std::vector<BYTE> BaremetalManager::get_code() {
    return this->code;
}

arion::CPU_ARCH BaremetalManager::get_arch() {
    return this->arch;
}

arion::ADDR BaremetalManager::get_load_addr() {
    return this->load_addr;
}

arion::ADDR BaremetalManager::get_entry_addr() {
    return this->entry_addr;
}