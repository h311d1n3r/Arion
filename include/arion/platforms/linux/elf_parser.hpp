#ifndef ARION_ELF_PARSER_HPP
#define ARION_ELF_PARSER_HPP

#include <arion/LIEF/ELF/NoteDetails/core/CoreFile.hpp>
#include <arion/LIEF/LIEF.hpp>
#include <arion/common/executable_parser.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <string>
#include <vector>

namespace arion
{

class Arion;

/// Enumeration of possible ELF file types relevant to the Arion emulator.
enum ELF_FILE_TYPE
{
    /// Unknown ELF file type.
    UNKNOWN_FILE,
    /// Relocatable file.
    REL,
    /// Executable file.
    EXEC,
    /// Shared object file (dynamic library).
    DYN,
    /// Core dump file.
    CORE,
    /// Sentinel for the number of types.
    NUM
};

/// Mapping from LIEF's internal ELF file type enumeration to Arion's `ELF_FILE_TYPE`.
extern std::map<LIEF::ELF::Header::FILE_TYPE, ELF_FILE_TYPE> lief_arion_file_types;

/// Structure holding register data for a single thread extracted from an ELF core dump.
struct ELF_COREDUMP_THREAD
{
    /// Raw byte data of the general-purpose register set (`prstatus`) note.
    std::vector<BYTE> raw_prstatus;
    /// Raw byte data of the floating-point register set (`fpregset`) note.
    std::vector<BYTE> raw_fpregset;

    /**
     * Builder for an ELF_COREDUMP_THREAD instance, initializing with `prstatus` data.
     * @param[in] raw_prstatus Raw byte data of the `prstatus` note.
     */
    ELF_COREDUMP_THREAD(std::vector<BYTE> raw_prstatus) : raw_prstatus(raw_prstatus) {};
};

/// Structure holding attributes extracted from an ELF core dump, primarily threads' register states.
struct ELF_COREDUMP_ATTRS
{
    /// List of threads extracted from the core dump.
    std::vector<std::unique_ptr<ELF_COREDUMP_THREAD>> threads;
};

/// Extended attributes structure for the ELF file parser.
struct ELF_PARSER_ATTRIBUTES : public EXECUTABLE_PARSER_ATTRIBUTES
{
    /// The specific type of the ELF file.
    ELF_FILE_TYPE type = ELF_FILE_TYPE::UNKNOWN_FILE;
    /// Offset of the program headers table.
    ADDR prog_headers_off = 0;
    /// Size of a single program header entry.
    size_t prog_headers_entry_sz = 0;
    /// Number of program header entries.
    size_t prog_headers_n = 0;
    /// Attributes specific to CORE dump files, or null otherwise.
    std::unique_ptr<ELF_COREDUMP_ATTRS> coredump = 0;
};

/// Helper class responsible for parsing the specific NOTE sections of an ELF core dump file.
class ElfCoredumpParser
{
  private:
    /// Weak pointer back to the main Arion instance.
    std::weak_ptr<Arion> arion;
    /// Shared pointer to the parser attributes, including core dump attributes.
    std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs;
    /// Flag indicating if the NT_PRPSINFO note has been found and parsed.
    bool found_prpsinfo = false;
    /**
     * Parses the NT_FILE note, which contains information about the mapped files (segments).
     * @param[in] note The LIEF Note structure containing the NT_FILE data.
     * @param[in] segments List of segments already loaded/known.
     */
    void parse_file_note(const LIEF::ELF::Note &note, std::vector<std::shared_ptr<struct SEGMENT>> segments);
    /**
     * Parses the NT_PRSTATUS note, containing general-purpose register state for a thread.
     * @param[in] note The LIEF Note structure containing the NT_PRSTATUS data.
     * @param[in] attrs Shared pointer to the parser attributes.
     */
    void parse_prstatus_note(const LIEF::ELF::Note &note, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs);
    /**
     * Parses the NT_PRPSINFO note, containing process status info (e.g., command line, PID, UID).
     * @param[in] note The LIEF Note structure containing the NT_PRPSINFO data.
     * @param[in] attrs Shared pointer to the parser attributes.
     */
    void parse_prpsinfo_note(const LIEF::ELF::Note &note, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs);
    /**
     * Parses the NT_FPREGSET note, containing floating-point register state for a thread.
     * @param[in] note The LIEF Note structure containing the NT_FPREGSET data.
     * @param[in] attrs Shared pointer to the parser attributes.
     */
    void parse_fpregset_note(const LIEF::ELF::Note &note, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs);

  public:
    /**
     * Builder for ElfCoredumpParser instances.
     * @param[in] arion Weak pointer to the main Arion instance.
     */
    ElfCoredumpParser(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Iterates over all notes in the core dump binary and extracts thread context and memory map information.
     * @param[in] elf The unique pointer to the LIEF ELF Binary representation.
     * @param[in,out] attrs The shared pointer to the attributes where parsed data will be stored.
     * @param[in] segments List of memory segments parsed from the loadable program headers.
     * @return The unique pointer to the LIEF ELF Binary.
     */
    std::unique_ptr<LIEF::ELF::Binary> parse_coredump_data(std::unique_ptr<LIEF::ELF::Binary> elf,
                                                           std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs,
                                                           std::vector<std::shared_ptr<struct SEGMENT>> segments);
};

/// Primary class for parsing ELF executables, shared objects, or core dumps. Inherits from ExecutableParser.
class ElfParser : public ExecutableParser
{
  private:
    /// Instance of the core dump parser used if the file type is CORE.
    std::unique_ptr<ElfCoredumpParser> coredump_parser;
    /**
     * Parses general ELF header data (magic, architecture, entry point, file type).
     * @param[in] elf The unique pointer to the LIEF ELF Binary representation.
     * @return The unique pointer to the LIEF ELF Binary.
     */
    std::unique_ptr<LIEF::ELF::Binary> parse_general_data(std::unique_ptr<LIEF::ELF::Binary> elf);
    /**
     * Parses the loadable segments (Program Headers of type PT_LOAD) and populates the segment list in attributes.
     * @param[in] elf The unique pointer to the LIEF ELF Binary representation.
     * @return The unique pointer to the LIEF ELF Binary.
     */
    std::unique_ptr<LIEF::ELF::Binary> parse_segments(std::unique_ptr<LIEF::ELF::Binary> elf);

  public:
    /**
     * Builder for ElfParser instances.
     * @param[in] arion Weak pointer to the main Arion instance.
     * @param[in] args Vector of command-line arguments, where the first element is the target file path.
     * @throws arion_exception::InvalidArgumentException if `args` is empty.
     */
    ElfParser(std::weak_ptr<Arion> arion, std::vector<std::string> args)
        : ExecutableParser(arion), coredump_parser(std::make_unique<ElfCoredumpParser>(arion))
    {
        this->attrs = std::make_shared<ELF_PARSER_ATTRIBUTES>();
        if (!args.size())
            throw arion_exception::InvalidArgumentException("Program arguments must at least contain target name.");
        this->attrs->path = args.at(0);
        this->attrs->args = args;
    };
    /**
     * Main method to process and parse the entire ELF file, including loading, segment parsing, and core dump analysis.
     */
    void process();
};

}; // namespace arion

#endif // ARION_ELF_PARSER_HPP
