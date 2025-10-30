#include <arion/LIEF/ELF/NoteDetails/core/CoreFile.hpp>
#include <arion/LIEF/ELF/NoteDetails/core/CorePrStatus.hpp>
#include <arion/LIEF/ELF/Segment.hpp>
#include <arion/LIEF/ELF/enums.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/logger.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_excepts.hpp>
#include <arion/utils/convert_utils.hpp>
#include <filesystem>
#include <memory>

using namespace arion;
using namespace arion_exception;

std::map<LIEF::ELF::Header::FILE_TYPE, ELF_FILE_TYPE> arion::lief_arion_file_types = {
    {LIEF::ELF::Header::FILE_TYPE::NONE, ELF_FILE_TYPE::UNKNOWN_FILE},
    {LIEF::ELF::Header::FILE_TYPE::REL, ELF_FILE_TYPE::REL},
    {LIEF::ELF::Header::FILE_TYPE::EXEC, ELF_FILE_TYPE::EXEC},
    {LIEF::ELF::Header::FILE_TYPE::DYN, ELF_FILE_TYPE::DYN},
    {LIEF::ELF::Header::FILE_TYPE::CORE, ELF_FILE_TYPE::CORE}};

void ElfCoredumpParser::parse_file_note(const LIEF::ELF::Note &note,
                                        std::vector<std::shared_ptr<struct arion::SEGMENT>> segments)
{
    if (!LIEF::ELF::CoreFile::classof(&note))
        return;

    const auto &nt_core_file = static_cast<const LIEF::ELF::CoreFile &>(note);
    for (auto &file_entry : nt_core_file)
    {
        for (std::shared_ptr<struct arion::SEGMENT> &seg : segments)
        {
            if (seg->virt_addr >= file_entry.start && seg->virt_addr < file_entry.end)
                seg->info = file_entry.path;
        }
    }
}

void ElfCoredumpParser::parse_prstatus_note(const LIEF::ELF::Note &note,
                                            std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs)
{
    if (!LIEF::ELF::CorePrStatus::classof(&note))
        return;

    const auto &nt_core_prstatus = static_cast<const LIEF::ELF::CorePrStatus &>(note);
    LIEF::span<const uint8_t> prstatus_desc = nt_core_prstatus.description();
    std::vector<BYTE> prstatus_content(prstatus_desc.begin(), prstatus_desc.end());
    attrs->coredump->threads.push_back(std::make_unique<ELF_COREDUMP_THREAD>(prstatus_content));
}

void ElfCoredumpParser::parse_prpsinfo_note(const LIEF::ELF::Note &note,
                                            std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (!LIEF::ELF::CorePrPsInfo::classof(&note))
        return;
    if (this->found_prpsinfo)
        throw MultiplePrPsInfoNotesException();
    this->found_prpsinfo = true;
    const auto &nt_core_prpsinfo = static_cast<const LIEF::ELF::CorePrPsInfo &>(note);
    const auto info = nt_core_prpsinfo.info();
    std::string program_name = info->args_stripped();
    if (program_name.length())
    {
        attrs->args = std::vector<std::string>({info->args_stripped()});
        attrs->path = program_name;
    }
    arion->set_pid(info->pid);
    arion->set_pgid(info->pgrp);
    arion->set_sid(info->sid);
    if (info->zombie)
        arion->set_zombie();
}

void ElfCoredumpParser::parse_fpregset_note(const LIEF::ELF::Note &note,
                                            std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs)
{
    size_t threads_sz = attrs->coredump->threads.size();
    if (!threads_sz)
        throw NoCoredumpCurrentThreadException();
    std::unique_ptr<ELF_COREDUMP_THREAD> thread = std::move(attrs->coredump->threads.at(threads_sz - 1));

    LIEF::span<const uint8_t> fpregset_desc = note.description();
    std::vector<BYTE> fpregset_content(fpregset_desc.begin(), fpregset_desc.end());
    thread->raw_fpregset = fpregset_content;

    attrs->coredump->threads[threads_sz - 1] = std::move(thread);
}

std::unique_ptr<LIEF::ELF::Binary> ElfCoredumpParser::parse_coredump_data(
    std::unique_ptr<LIEF::ELF::Binary> elf, std::shared_ptr<ELF_PARSER_ATTRIBUTES> attrs,
    std::vector<std::shared_ptr<struct arion::SEGMENT>> segments)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    attrs->coredump = std::make_unique<ELF_COREDUMP_ATTRS>();
    for (const LIEF::ELF::Note &note : elf->notes())
    {
        switch (note.type())
        {
        case LIEF::ELF::Note::TYPE::CORE_FILE:
            this->parse_file_note(note, segments);
            break;
        case LIEF::ELF::Note::TYPE::CORE_PRSTATUS:
            this->parse_prstatus_note(note, attrs);
            break;
        case LIEF::ELF::Note::TYPE::CORE_PRPSINFO:
            this->parse_prpsinfo_note(note, attrs);
            break;
        case LIEF::ELF::Note::TYPE::CORE_FPREGSET:
            this->parse_fpregset_note(note, attrs);
            break;
        default: {
            colorstream cs;
            cs << LOG_COLOR::ORANGE << "Coredump note " << LOG_COLOR::MAGENTA
               << int_to_hex<uint32_t>(note.original_type()) << LOG_COLOR::ORANGE << " is not implemented.";
            arion->logger->warn(cs.str());
            break;
        }
        }
    }
    return std::move(elf);
}

void ElfParser::process()
{
    if (!std::filesystem::exists(this->attrs->path))
        throw FileNotFoundException(this->attrs->path);
    this->attrs->usr_path = this->attrs->path; // "usr_path" is the path passed by the user to Arion whereas "path" can
                                               // be resolved to another path (e.g in core dumps)

    auto elf_attrs = std::dynamic_pointer_cast<ELF_PARSER_ATTRIBUTES>(this->attrs);

    std::unique_ptr<LIEF::ELF::Binary> elf = LIEF::ELF::Parser::parse(this->attrs->path);
    if (!elf)
        throw ElfParsingException(this->attrs->path);
    elf = this->parse_general_data(std::move(elf));
    elf = this->parse_segments(std::move(elf));
    if (elf_attrs->type == ELF_FILE_TYPE::CORE)
        elf = this->coredump_parser->parse_coredump_data(std::move(elf), elf_attrs, this->segments);
}

std::unique_ptr<LIEF::ELF::Binary> ElfParser::parse_general_data(std::unique_ptr<LIEF::ELF::Binary> elf)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    auto elf_attrs = std::dynamic_pointer_cast<ELF_PARSER_ATTRIBUTES>(this->attrs);

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
    auto elf_attrs = std::dynamic_pointer_cast<ELF_PARSER_ATTRIBUTES>(this->attrs);

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
