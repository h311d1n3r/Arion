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

enum ELF_FILE_TYPE
{
    UNKNOWN_FILE,
    REL,
    EXEC,
    DYN,
    CORE,
    NUM
};

extern std::map<LIEF::ELF::Header::FILE_TYPE, ELF_FILE_TYPE> lief_arion_file_types;

struct ELF_COREDUMP_THREAD
{
    std::vector<BYTE> raw_prstatus;
    std::vector<BYTE> raw_fpregset;

    ELF_COREDUMP_THREAD(std::vector<BYTE> raw_prstatus) : raw_prstatus(raw_prstatus) {};
};

struct ELF_COREDUMP_ATTRS
{
    std::vector<std::unique_ptr<ELF_COREDUMP_THREAD>> threads;
};

struct ELF_PARSER_ATTRIBUTES : public EXECUTABLE_PARSER_ATTRIBUTES
{
    ELF_FILE_TYPE type = ELF_FILE_TYPE::UNKNOWN_FILE;
    ADDR prog_headers_off = 0;
    size_t prog_headers_entry_sz = 0;
    size_t prog_headers_n = 0;
    std::unique_ptr<ELF_COREDUMP_ATTRS> coredump = 0;
};

class ElfCoredumpParser
{
  private:
    std::weak_ptr<Arion> arion;
    std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs;
    bool found_prpsinfo = false;
    void parse_file_note(const LIEF::ELF::Note &note, std::vector<std::shared_ptr<struct SEGMENT>> segments);
    void parse_prstatus_note(const LIEF::ELF::Note &note, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs);
    void parse_prpsinfo_note(const LIEF::ELF::Note &note, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs);
    void parse_fpregset_note(const LIEF::ELF::Note &note, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs);

  public:
    ElfCoredumpParser(std::weak_ptr<Arion> arion) : arion(arion) {};
    std::unique_ptr<LIEF::ELF::Binary> parse_coredump_data(std::unique_ptr<LIEF::ELF::Binary> elf,
                                                           std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs,
                                                           std::vector<std::shared_ptr<struct SEGMENT>> segments);
};

class ElfParser : public ExecutableParser
{
  private:
    std::unique_ptr<ElfCoredumpParser> coredump_parser;
    std::unique_ptr<LIEF::ELF::Binary> parse_general_data(std::unique_ptr<LIEF::ELF::Binary> elf);
    std::unique_ptr<LIEF::ELF::Binary> parse_segments(std::unique_ptr<LIEF::ELF::Binary> elf);

  public:
    ElfParser(std::weak_ptr<Arion> arion, std::vector<std::string> args)
        : ExecutableParser(arion), coredump_parser(std::make_unique<ElfCoredumpParser>(arion))
    {
        this->attrs = std::make_shared<ELF_PARSER_ATTRIBUTES>();
        if (!args.size())
            throw arion_exception::InvalidArgumentException("Program arguments must at least contain target name.");
        this->attrs->path = args.at(0);
        this->attrs->args = args;
    };
    void process();
};

}; // namespace arion

#endif // ARION_ELF_PARSER_HPP
