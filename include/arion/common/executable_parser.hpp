#ifndef ARION_EXECUTABLE_PARSER_HPP
#define ARION_EXECUTABLE_PARSER_HPP

#include <arion/LIEF/LIEF.hpp>
#include <arion/common/global_defs.hpp>
#include <memory>
#include <string>

namespace arion
{

class Arion;

/// Stores attributes which are filled during the parsing of an executable.
struct EXECUTABLE_PARSER_ATTRIBUTES
{
    /**
     * Destructor for EXECUTABLE_PARSER_ATTRIBUTES instances.
     */
    virtual ~EXECUTABLE_PARSER_ATTRIBUTES() = default;
    /// Path to the executable as passed to the Arion instance when initializing it.
    std::string usr_path;
    /// Path to the executable that may differ from the user specified one in some special cases (like coredumps).
    std::string path;
    /// The program arguments of the emulated process, basically the "argv" array.
    std::vector<std::string> args;
    /// The CPU architecture of the executable.
    CPU_ARCH arch = CPU_ARCH::UNKNOWN_ARCH;
    /// The method used to link the executable.
    LINKAGE_TYPE linkage = LINKAGE_TYPE::UNKNOWN_LINKAGE;
    /// Path to the interpreter of the executable.
    std::string interpreter_path;
    /// Memory address at which the executable starts being executed.
    ADDR entry = 0;
};

/// This class is responsible for extracting specific data from an executable file.
class ExecutableParser
{
  protected:
    /// The Arion instance which requested the executable parsing.
    std::weak_ptr<Arion> arion;
    /// A structure of attributes which are filled during the parsing.
    std::shared_ptr<EXECUTABLE_PARSER_ATTRIBUTES> attrs;
    /// The memory segments in the executable.
    std::vector<std::shared_ptr<struct SEGMENT>> segments;

  public:
    /**
     * Builder for ExecutableParser instances.
     * @param[in] arion The Arion instance which requested the executable parsing.
     */
    ExecutableParser(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Processes the requested executable file by parsing its format and extracting some specific data.
     */
    virtual void process() = 0;
    /**
     * Retrieves the structure of attributes which were filled during parsing.
     * @return The structure of attributes.
     */
    std::shared_ptr<EXECUTABLE_PARSER_ATTRIBUTES> get_attrs();
    /**
     * Retrieves the list of memory segments which where extracted from the executable.
     * @return The list of memory segments.
     */
    std::vector<std::shared_ptr<struct SEGMENT>> get_segments();
};

}; // namespace arion

#endif // ARION_EXECUTABLE_PARSER_HPP
