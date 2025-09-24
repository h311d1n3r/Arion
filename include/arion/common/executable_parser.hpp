#ifndef ARION_EXECUTABLE_PARSER_HPP
#define ARION_EXECUTABLE_PARSER_HPP

#include <arion/LIEF/LIEF.hpp>
#include <arion/common/global_defs.hpp>
#include <memory>
#include <string>

namespace arion
{

class Arion;

struct EXECUTABLE_PARSER_ATTRIBUTES
{
    virtual ~EXECUTABLE_PARSER_ATTRIBUTES() = default;
    std::string usr_path;
    std::string path;
    std::vector<std::string> args;
    CPU_ARCH arch = CPU_ARCH::UNKNOWN_ARCH;
    LINKAGE_TYPE linkage = LINKAGE_TYPE::UNKNOWN_LINKAGE;
    std::string interpreter_path;
    ADDR entry = 0;
};

class ExecutableParser
{
  protected:
    std::weak_ptr<Arion> arion;
    std::shared_ptr<EXECUTABLE_PARSER_ATTRIBUTES> attrs;
    std::vector<std::shared_ptr<struct SEGMENT>> segments;

  public:
    ExecutableParser(std::weak_ptr<Arion> arion) : arion(arion) {};
    virtual void process() = 0;
    std::shared_ptr<EXECUTABLE_PARSER_ATTRIBUTES> get_attrs();
    std::vector<std::shared_ptr<struct SEGMENT>> get_segments();
};

}; // namespace arion

#endif // ARION_EXECUTABLE_PARSER_HPP
