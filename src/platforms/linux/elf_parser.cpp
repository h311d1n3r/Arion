#include <LIEF/ELF/Segment.hpp>
#include <LIEF/ELF/enums.hpp>
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
    if (!std::filesystem::exists(this->elf_path))
        throw FileNotFoundException(this->elf_path);
    std::unique_ptr<const LIEF::ELF::Binary> elf = LIEF::ELF::Parser::parse(this->elf_path);
    if (!elf)
        throw ElfParsingException(this->elf_path);
    elf = this->parse_general_data(std::move(elf));
    elf = this->parse_segments(std::move(elf));
}

std::unique_ptr<const LIEF::ELF::Binary> ElfParser::parse_general_data(std::unique_ptr<const LIEF::ELF::Binary> elf)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    this->linkage = LINKAGE_TYPE::DYNAMIC_LINKAGE;
    this->type = lief_arion_file_types.at(elf->header().file_type());
    if (elf->has_interpreter())
        this->interpreter = arion->fs->to_fs_path(std::string(elf->interpreter()));
    else
        this->linkage = LINKAGE_TYPE::STATIC_LINKAGE;
    this->entry = elf->entrypoint();
    this->prog_headers_off = elf->header().program_headers_offset();
    this->prog_headers_entry_sz = elf->header().program_header_size();
    this->prog_headers_n = elf->header().numberof_segments();
    LIEF::ELF::ARCH lief_arch = elf->header().machine_type();
    switch (lief_arch)
    {
    case LIEF::ELF::ARCH::I386:
        this->arch = CPU_ARCH::X86_ARCH;
        break;
    case LIEF::ELF::ARCH::X86_64:
        this->arch = CPU_ARCH::X8664_ARCH;
        break;
    case LIEF::ELF::ARCH::ARM:
        this->arch = CPU_ARCH::ARM_ARCH;
        break;
    case LIEF::ELF::ARCH::AARCH64:
        this->arch = CPU_ARCH::ARM64_ARCH;
        break;
    default:
        throw UnsupportedCpuArchException();
    }
    return std::move(elf);
}

std::unique_ptr<const LIEF::ELF::Binary> ElfParser::parse_segments(std::unique_ptr<const LIEF::ELF::Binary> elf)
{
    for (const LIEF::ELF::Segment &lief_seg : elf->segments())
    {
        if (lief_seg.type() != LIEF::ELF::Segment::TYPE::LOAD)
            continue;
        std::unique_ptr<struct arion::SEGMENT> seg = std::make_unique<struct arion::SEGMENT>();
        seg->virt_addr = lief_seg.virtual_address();
        seg->file_addr = lief_seg.file_offset();
        seg->align = lief_seg.alignment();
        seg->virt_sz = lief_seg.virtual_size();
        seg->phy_sz = lief_seg.physical_size();
        seg->flags = (lief_seg.has(LIEF::ELF::Segment::FLAGS::R) << 2) |
                     (lief_seg.has(LIEF::ELF::Segment::FLAGS::W) << 1) | lief_seg.has(LIEF::ELF::Segment::FLAGS::X);
        this->segments.push_back(std::move(seg));
    }
    return std::move(elf);
}
