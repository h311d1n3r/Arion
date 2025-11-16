#include <arion/common/global_excepts.hpp>

namespace arion_exception
{

/// Exception thrown when an error occurs while reading a file expected to be a symbolic link target.
class ReadLinkFileException : public ArionException
{
  public:
    /**
     * Builder for ReadLinkFileException instances.
     * @param[in] file_path Path to the link file that caused the error.
     */
    explicit ReadLinkFileException(std::string file_path)
        : ArionException(std::string("An error occurred while reading \"") + file_path +
                         std::string("\" as a link file.")) {};
};

/// Exception thrown when a general error occurs during the parsing of an ELF file.
class ElfParsingException : public ArionException
{
  public:
    /**
     * Builder for ElfParsingException instances.
     * @param[in] elf_path Path to the ELF file that caused the parsing error.
     */
    explicit ElfParsingException(std::string elf_path)
        : ArionException(std::string("An error occurred when parsing ELF \"") + elf_path + std::string("\".")) {};
};

/// Exception thrown when parsing an ELF core dump that contains process status information for more than one process.
class MultiplePrPsInfoNotesException : public ArionException
{
  public:
    /**
     * Builder for MultiplePrPsInfoNotesException instances.
     */
    explicit MultiplePrPsInfoNotesException()
        : ArionException(std::string("Coredump file contains information for more than one process.")) {};
};

}; // namespace arion_exception
