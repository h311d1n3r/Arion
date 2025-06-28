#ifndef ARION_ELF_PARSER_HPP
#define ARION_ELF_PARSER_HPP

#include <arion/LIEF/ELF/NoteDetails/core/CoreFile.hpp>
#include <arion/LIEF/LIEF.hpp>
#include <arion/common/executable_parser.hpp>
#include <arion/common/global_defs.hpp>
#include <string>

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

struct ARION_ELF_PARSER_ATTRIBUTES : public ARION_EXECUTABLE_PARSER_ATTRIBUTES
{
    ELF_FILE_TYPE type;
    arion::ADDR prog_headers_off;
    size_t prog_headers_entry_sz;
    size_t prog_headers_n;
};

class ElfParser : public ExecutableParser
{
  private:
    std::unique_ptr<LIEF::ELF::Binary> parse_general_data(std::unique_ptr<LIEF::ELF::Binary> elf);
    std::unique_ptr<LIEF::ELF::Binary> parse_segments(std::unique_ptr<LIEF::ELF::Binary> elf);
    std::unique_ptr<LIEF::ELF::Binary> parse_coredump_data(std::unique_ptr<LIEF::ELF::Binary> elf);

  public:
    ElfParser(std::weak_ptr<Arion> arion, std::string elf_path) : ExecutableParser(arion)
    {
        this->attrs = std::make_shared<ARION_ELF_PARSER_ATTRIBUTES>();
        this->attrs->path = elf_path;
    };
    void process();
};

#endif // ARION_ELF_PARSER_HPP
