#include <arion/arion.hpp>
#include <arion/utils/convert_utils.hpp>
#include <arion/utils/type_utils.hpp>
#include <arion/utils/struct_utils.hpp>

using namespace arion;

// Common types declaration
std::shared_ptr<ArionIntType> ARION_INT_TYPE;
std::shared_ptr<ArionRawStringType> ARION_RAW_STRING_TYPE;

// Common types registration
REGISTER_ARION_TYPE(ARION_INT_TYPE, ArionIntType, COMMON_TYPE_PRIORITY)
REGISTER_ARION_TYPE(ARION_RAW_STRING_TYPE, ArionRawStringType, COMMON_TYPE_PRIORITY)

std::string ArionType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    return int_to_hex<uint64_t>(val);
}

ARION_LOG_COLOR ArionType::get_color()
{
    return this->color;
}

std::string ArionFlagType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    if(!val) return int_to_hex<uint64_t>(0);
    std::string res = "";
    for(uint8_t i = 0; i < sizeof(uint64_t) * 8; i++) {
        uint64_t flag_val = ((uint64_t) 1 << i);
        if(!(val & flag_val))
            continue;
        if(res.length()) res += " | ";
        auto flag_it = this->flag_map.find(flag_val);
        if(flag_it == this->flag_map.end())
            res += int_to_hex<uint64_t>(flag_val);
        else
            res += flag_it->second;
    }
    return res;
}

bool arion_poly_struct::AbsArionStructType::arion_is_mapped(std::shared_ptr<Arion> arion, arion::ADDR addr) {
    return arion->mem->is_mapped(addr);
}

CPU_ARCH arion_poly_struct::AbsArionStructType::arion_curr_arch(std::shared_ptr<Arion> arion) {
    return arion->abi->get_attrs()->arch;
}

std::vector<arion::BYTE> arion_poly_struct::AbsArionStructType::arion_read_mem(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz) {
    return arion->mem->read(addr, sz);
}

std::string ArionRawStringType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    return std::string("\"") + arion->mem->read_c_string(val) + std::string("\"");
}
