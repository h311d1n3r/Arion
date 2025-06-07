#ifndef ARION_TYPE_UTILS_HPP
#define ARION_TYPE_UTILS_HPP

#include <arion/common/logger.hpp>
#include <arion/utils/struct_utils.hpp>
#include <cstdint>
#include <memory>
#include <string>

class Arion;

class ArionType
{
  private:
    std::string name;
    ARION_LOG_COLOR color;

  protected:
    ArionType(std::string name, ARION_LOG_COLOR color = ARION_LOG_COLOR::DEFAULT) : name(name), color(color) {};

  public:
    ARION_LOG_COLOR get_color();
    virtual std::string str(std::shared_ptr<Arion> arion, uint64_t val);
};

class ArionFlagType : public ArionType {
private:
    std::map<uint64_t, std::string> flag_map;

protected:
    ArionFlagType(std::string name, std::map<uint64_t, std::string> flag_map) : ArionType(name, ARION_LOG_COLOR::CYAN), flag_map(flag_map) {};

public:
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};

class AbsArionStructType {
protected:
    bool arion_is_mapped(std::shared_ptr<Arion> arion, arion::ADDR addr);
    arion::CPU_ARCH arion_curr_arch(std::shared_ptr<Arion> arion);
    std::vector<arion::BYTE> arion_read_mem(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz);
};

template <typename T>
class ArionStructType : public ArionType, AbsArionStructType {
private:
    std::shared_ptr<arion_poly_struct::PolymorphicStructFactory<T>> factory;

protected:
    ArionStructType(std::string name, std::shared_ptr<arion_poly_struct::PolymorphicStructFactory<T>> factory) : ArionType(name, ARION_LOG_COLOR::BLUE), factory(factory) {};

public:
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override {
        if(!val || !this->arion_is_mapped(arion, val))
            return int_to_hex<uint64_t>(val);
        arion::CPU_ARCH arch = this->arion_curr_arch(arion);
        size_t struct_sz = this->factory->get_struct_sz(arch);
        arion::BYTE* struct_buf = (arion::BYTE*) malloc(struct_sz);
        std::vector<arion::BYTE> struct_data = this->arion_read_mem(arion, val, struct_sz);
        memcpy(struct_buf, struct_data.data(), struct_sz);
        arion_poly_struct::STRUCT_ID struct_id = this->factory->feed(arch, struct_buf);
        std::string struct_str = this->factory->to_string(struct_id, arch);
        free(struct_buf);
        this->factory->release_struct(struct_id);
        return struct_str;
    }
};

class ArionIntType : public ArionType
{
  public:
    ArionIntType() : ArionType("Int", ARION_LOG_COLOR::MAGENTA) {};
};
extern std::shared_ptr<ArionIntType> ARION_INT_TYPE;

class ArionRawStringType : public ArionType
{
  public:
    ArionRawStringType() : ArionType("Raw String", ARION_LOG_COLOR::GREEN) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern std::shared_ptr<ArionRawStringType> ARION_RAW_STRING_TYPE;

#endif // ARION_TYPE_UTILS_HPP
