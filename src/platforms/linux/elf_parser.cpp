#include <arion/LIEF/ELF/NoteDetails/core/CoreFile.hpp>
#include <arion/LIEF/ELF/Segment.hpp>
#include <arion/LIEF/ELF/enums.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <filesystem>
#include <memory>

using namespace arion;

std::map<LIEF::ELF::Header::FILE_TYPE, ELF_FILE_TYPE> lief_arion_file_types = {
    {LIEF::ELF::Header::FILE_TYPE::NONE, ELF_FILE_TYPE::UNKNOWN_FILE},
    {LIEF::ELF::Header::FILE_TYPE::REL, ELF_FILE_TYPE::REL},
    {LIEF::ELF::Header::FILE_TYPE::EXEC, ELF_FILE_TYPE::EXEC},
    {LIEF::ELF::Header::FILE_TYPE::DYN, ELF_FILE_TYPE::DYN},
    {LIEF::ELF::Header::FILE_TYPE::CORE, ELF_FILE_TYPE::CORE}};

void ElfParser::process()
{
    if (!std::filesystem::exists(this->attrs->path))
        throw FileNotFoundException(this->attrs->path);

    auto elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(this->attrs);

    std::unique_ptr<LIEF::ELF::Binary> elf = LIEF::ELF::Parser::parse(this->attrs->path);
    if (!elf)
        throw ElfParsingException(this->attrs->path);
    elf = this->parse_general_data(std::move(elf));
    elf = this->parse_segments(std::move(elf));
    if (elf_attrs->type == ELF_FILE_TYPE::CORE)
        elf = this->parse_coredump_data(std::move(elf));
}

std::unique_ptr<LIEF::ELF::Binary> ElfParser::parse_general_data(std::unique_ptr<LIEF::ELF::Binary> elf)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    auto elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(this->attrs);

    elf_attrs->linkage = LINKAGE_TYPE::DYNAMIC_LINKAGE;
    elf_attrs->type = lief_arion_file_types.at(elf->header().file_type());
    if (elf->has_interpreter())
        elf_attrs->interpreter_path = arion->fs->to_fs_path(std::string(elf->interpreter()));
    else
        elf_attrs->linkage = LINKAGE_TYPE::STATIC_LINKAGE;
    elf_attrs->entry = elf->entrypoint();
    elf_attrs->prog_headers_off = elf->header().program_headers_offset();
    elf_attrs->prog_headers_entry_sz = elf->header().program_header_size();
    elf_attrs->prog_headers_n = elf->header().numberof_segments();
    LIEF::ELF::ARCH lief_arch = elf->header().machine_type();
    switch (lief_arch)
    {
    case LIEF::ELF::ARCH::I386:
        elf_attrs->arch = CPU_ARCH::X86_ARCH;
        break;
    case LIEF::ELF::ARCH::X86_64:
        elf_attrs->arch = CPU_ARCH::X8664_ARCH;
        break;
    case LIEF::ELF::ARCH::ARM:
        elf_attrs->arch = CPU_ARCH::ARM_ARCH;
        break;
    case LIEF::ELF::ARCH::AARCH64:
        elf_attrs->arch = CPU_ARCH::ARM64_ARCH;
        break;
    default:
        throw UnsupportedCpuArchException();
    }
    return std::move(elf);
}

std::unique_ptr<LIEF::ELF::Binary> ElfParser::parse_segments(std::unique_ptr<LIEF::ELF::Binary> elf)
{
    auto elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(this->attrs);

    for (const LIEF::ELF::Segment &lief_seg : elf->segments())
    {
        if (lief_seg.type() != LIEF::ELF::Segment::TYPE::LOAD)
            continue;
        std::shared_ptr<struct arion::SEGMENT> seg = std::make_shared<struct arion::SEGMENT>();
        seg->info = elf_attrs->type == ELF_FILE_TYPE::CORE ? "" : elf_attrs->path;
        seg->virt_addr = lief_seg.virtual_address();
        seg->file_addr = lief_seg.file_offset();
        seg->align = lief_seg.alignment();
        seg->virt_sz = lief_seg.virtual_size();
        seg->phy_sz = lief_seg.physical_size();
        seg->flags = (lief_seg.has(LIEF::ELF::Segment::FLAGS::R) << 2) |
                     (lief_seg.has(LIEF::ELF::Segment::FLAGS::W) << 1) | lief_seg.has(LIEF::ELF::Segment::FLAGS::X);
        this->segments.push_back(seg);
    }
    return std::move(elf);
}

std::unique_ptr<LIEF::ELF::Binary> ElfParser::parse_coredump_data(std::unique_ptr<LIEF::ELF::Binary> elf)
{
    for (const LIEF::ELF::Note &note : elf->notes())
    {
        if (LIEF::ELF::CoreFile::classof(&note))
        {
            const auto &nt_core_file = static_cast<const LIEF::ELF::CoreFile &>(note);
            for (auto &file_entry : nt_core_file)
            {
                for (std::shared_ptr<struct arion::SEGMENT> &seg : this->segments)
                {
                    if (seg->virt_addr >= file_entry.start && seg->virt_addr < file_entry.end)
                        seg->info = file_entry.path;
                }
            }
        }
    }
    return std::move(elf);
}
