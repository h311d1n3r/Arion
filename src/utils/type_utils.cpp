#include <arion/arion.hpp>
#include <arion/utils/convert_utils.hpp>
#include <arion/utils/type_utils.hpp>

using namespace arion;

ArionIntType ARION_INT_TYPE;
ArionRawStringType ARION_RAW_STRING_TYPE;

std::string ArionType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    return int_to_hex<uint64_t>(val);
}

ARION_LOG_COLOR ArionType::get_color()
{
    return this->color;
}

std::string ArionRawStringType::str(std::shared_ptr<Arion> arion, uint64_t val)
{
    return arion->mem->read_c_string(val);
}
