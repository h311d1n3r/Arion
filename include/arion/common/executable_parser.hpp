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
    std::string path;
    arion::CPU_ARCH arch;
    arion::LINKAGE_TYPE linkage;
    std::string interpreter_path;
    arion::ADDR entry;
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
