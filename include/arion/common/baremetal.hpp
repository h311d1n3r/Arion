#ifndef ARION_BAREMETAL_HPP
#define ARION_BAREMETAL_HPP

#include <arion/common/global_defs.hpp>
#include <any>

class ARION_EXPORT Baremetal {
private:
    std::map<std::string, std::any> baremetal_map = {
        {"coderaw", std::make_shared<std::vector<uint8_t>>()},
        {"bitsize", 64},
        {"arch", arion::CPU_ARCH::X8664_ARCH},
        {"setup_memory", false},
    };
public:

    template <typename T>
    void ARION_EXPORT set_field(const std::string& key, T value) {
        auto it = baremetal_map.find(key);
        if (it == baremetal_map.end())
            throw BaremetalKeyNotFoundException(key);
        it->second = value;
    }

    template <typename T>
    T ARION_EXPORT get_field(const std::string& key) const {
        auto it = baremetal_map.find(key);
        if (it == baremetal_map.end())
            throw BaremetalKeyNotFoundException(key);
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            throw BaremetalWrongTypeAccessException(key);
        }
    }

    Baremetal ARION_EXPORT clone() const {
        Baremetal baremetal;
        baremetal.baremetal_map = this->baremetal_map;
        return baremetal;
    }

    void ARION_EXPORT init_default_instance(std::shared_ptr<Arion> arion);
};


#endif // ARION_BAREMETAL_HPP
