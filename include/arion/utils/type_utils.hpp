#ifndef ARION_TYPE_UTILS_HPP
#define ARION_TYPE_UTILS_HPP

#include <arion/common/logger.hpp>
#include <cstdint>
#include <memory>
#include <string>

class Arion;

class ArionType
{
  private:
    std::string name;
    ARION_LOG_COLOR color;

  public:
    ArionType(std::string name, ARION_LOG_COLOR color = ARION_LOG_COLOR::DEFAULT) : name(name), color(color) {};
    ARION_LOG_COLOR get_color();
    virtual std::string str(std::shared_ptr<Arion> arion, uint64_t val);
};

class ArionIntType : public ArionType
{
  public:
    ArionIntType() : ArionType("Int", ARION_LOG_COLOR::MAGENTA) {};
};
extern ArionIntType ARION_INT_TYPE;

class ArionRawStringType : public ArionType
{
  public:
    ArionRawStringType() : ArionType("Raw String", ARION_LOG_COLOR::GREEN) {};
    std::string str(std::shared_ptr<Arion> arion, uint64_t val) override;
};
extern ArionRawStringType ARION_RAW_STRING_TYPE;

#endif // ARION_TYPE_UTILS_HPP
