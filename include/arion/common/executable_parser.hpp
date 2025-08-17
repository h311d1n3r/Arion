#ifndef ARION_EXECUTABLE_PARSER_HPP
#define ARION_EXECUTABLE_PARSER_HPP

#include <arion/LIEF/LIEF.hpp>
#include <arion/common/global_defs.hpp>
#include <memory>
#include <string>

class Arion;

struct ARION_EXECUTABLE_PARSER_ATTRIBUTES
{
    virtual ~ARION_EXECUTABLE_PARSER_ATTRIBUTES() = default;
    std::string usr_path;
    std::string path;
    std::vector<std::string> args;
    arion::CPU_ARCH arch = arion::CPU_ARCH::UNKNOWN_ARCH;
    arion::LINKAGE_TYPE linkage = arion::LINKAGE_TYPE::UNKNOWN_LINKAGE;
    std::string interpreter_path;
    arion::ADDR entry = 0;
};

class ExecutableParser
{
  protected:
    std::weak_ptr<Arion> arion;
    std::shared_ptr<ARION_EXECUTABLE_PARSER_ATTRIBUTES> attrs;
    std::vector<std::shared_ptr<struct arion::SEGMENT>> segments;

  public:
    ExecutableParser(std::weak_ptr<Arion> arion) : arion(arion) {};
    virtual void process() = 0;
    std::shared_ptr<ARION_EXECUTABLE_PARSER_ATTRIBUTES> get_attrs();
    std::vector<std::shared_ptr<struct arion::SEGMENT>> get_segments();
};

#endif // ARION_EXECUTABLE_PARSER_HPP
