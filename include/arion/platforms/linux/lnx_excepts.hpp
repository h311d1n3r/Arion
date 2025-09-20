#include <arion/common/global_excepts.hpp>

namespace arion
{

class ReadLinkFileException : public ArionException
{
  public:
    explicit ReadLinkFileException(std::string file_path)
        : ArionException(std::string("An error occurred while reading \"") + file_path +
                         std::string("\" as a link file.")) {};
};

class ElfParsingException : public ArionException
{
  public:
    explicit ElfParsingException(std::string elf_path)
        : ArionException(std::string("An error occurred when parsing ELF \"") + elf_path + std::string("\".")) {};
};

class MultiplePrPsInfoNotesException : public ArionException
{
  public:
    explicit MultiplePrPsInfoNotesException()
        : ArionException(std::string("Coredump file contains information for more than one process.")) {};
};

}; // namespace arion
