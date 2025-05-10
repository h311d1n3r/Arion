#ifndef ARION_ELF_PARSER_HPP
#define ARION_ELF_PARSER_HPP

#include <arion/LIEF/LIEF.hpp>
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

class ElfParser
{
  private:
    std::weak_ptr<Arion> arion;
    std::unique_ptr<LIEF::ELF::Binary> parse_general_data(std::unique_ptr<LIEF::ELF::Binary> elf);
    std::unique_ptr<LIEF::ELF::Binary> parse_segments(std::unique_ptr<LIEF::ELF::Binary> elf);

  public:
    const std::string elf_path;
    std::vector<std::unique_ptr<struct arion::SEGMENT>> segments;
    arion::LINKAGE_TYPE linkage;
    ELF_FILE_TYPE type;
    std::string interpreter;
    arion::ADDR entry;
    arion::ADDR prog_headers_off;
    size_t prog_headers_entry_sz;
    size_t prog_headers_n;
    arion::CPU_ARCH arch;
    ElfParser(std::weak_ptr<Arion> arion, std::string elf_path) : arion(arion), elf_path(elf_path) {};
    void process();
};

#endif // ARION_ELF_PARSER_HPP
