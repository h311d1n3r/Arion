#include <arion/archs/arch_x86-64.hpp>
#include <arion/arion.hpp>
#include <arion/common/arch_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/utils/fs_utils.hpp>
#include <arion/utils/math_utils.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <unistd.h>

using namespace arion;

std::unique_ptr<LNX_LOADER_PARAMS> ElfLoader::process()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->arch->get_attrs()->arch_sz;
    auto prog_elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(prog_parser->get_attrs());

    std::shared_ptr<LNX_LOADER_PARAMS> params = std::make_shared<LNX_LOADER_PARAMS>();
    this->is_pie = prog_elf_attrs->type == ELF_FILE_TYPE::DYN;
    this->is_static = !this->interp_parser;
    params->load_address = this->map_elf_segments(
        this->prog_parser, this->is_pie ? (arch_sz == 64 ? LINUX_64_LOAD_ADDR : LINUX_32_LOAD_ADDR) : 0);
    if (!this->is_static)
        params->interp_address =
            this->map_elf_segments(this->interp_parser, arch_sz == 64 ? LINUX_64_INTERP_ADDR : LINUX_32_INTERP_ADDR);

    KERNEL_SEG_FLAGS seg_flags = arion->arch->get_attrs()->seg_flags;
    if (seg_flags & ARION_VVAR_PRESENT)
        params->vvar_address = this->map_vvar();
    else
        params->vvar_address = 0;
    if (seg_flags & ARION_VDSO_PRESENT)
        params->vdso_address = this->map_vdso();
    else
        params->vdso_address = 0;
    params->stack_address = this->map_stack(params, prog_elf_attrs->path);
    if (seg_flags & ARION_VSYSCALL_PRESENT)
        params->vsyscall_address = this->map_vsyscall();
    else
        params->vsyscall_address = 0;
    if (seg_flags & ARION_ARM_TRAPS_PRESENT)
        params->arm_traps_address = this->map_arm_traps();
    else
        params->arm_traps_address = 0;

    ADDR entry_addr;
    if (!this->is_static)
    {
        auto interp_elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(interp_parser->get_attrs());
        entry_addr = interp_elf_attrs->entry + params->interp_address;
    }
    else
        entry_addr = this->is_pie ? prog_elf_attrs->entry + params->load_address : prog_elf_attrs->entry;

    this->init_main_thread(params, entry_addr);
    return std::make_unique<LNX_LOADER_PARAMS>(*params.get());
}

ADDR ElfLoader::map_elf_segments(const std::shared_ptr<ElfParser> parser, ADDR load_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(parser->get_attrs());
    const std::string program_name = elf_attrs->path;

    ADDR first_addr = ARION_MAX_U64, last_addr = 0;
    for (const std::shared_ptr<struct SEGMENT> seg : parser->get_segments())
    {
        ADDR seg_data_start_addr = seg->virt_addr + load_addr;
        ADDR seg_start_addr = seg_data_start_addr;
        ADDR seg_end_addr = seg_start_addr + seg->virt_sz;
        if (seg->align > 1)
        {
            seg_start_addr -= (seg_start_addr % seg->align);
            seg_end_addr += (seg->align - (seg_end_addr % seg->align));
        }
        size_t data_seg_sz = seg_end_addr - seg_data_start_addr;
        if (seg->phy_sz > data_seg_sz)
            seg_end_addr += seg->align;
        size_t seg_sz = seg_end_addr - seg_start_addr;
        if (seg_end_addr > last_addr)
            last_addr = seg_end_addr;
        if (seg_start_addr < first_addr)
            first_addr = seg_start_addr;

        arion->mem->map(seg_start_addr, seg_sz, seg->flags, seg->info);

        std::shared_ptr<Arion> arion_cpy = arion;
        RD_BIN_CALLBACK on_file_read = [arion_cpy, seg_data_start_addr](std::array<BYTE, ARION_BUF_SZ> buf, ADDR off,
                                                                        size_t sz) {
            arion_cpy->mem->write(seg_data_start_addr + off, buf.data(), sz);
        };
        read_bin_file(program_name, seg->file_addr, seg->phy_sz, on_file_read);
    }
    if (parser == this->prog_parser)
    {
        arion->mem->map(last_addr, HEAP_SZ, HEAP_PERMS, "[heap]");
        arion->mem->set_brk(last_addr + HEAP_SZ);
    }
    return first_addr;
}

ADDR ElfLoader::map_vvar()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->arch->get_attrs()->arch_sz;

    ADDR vvar_load_addr = arch_sz == 64 ? LINUX_64_VVAR_ADDR : LINUX_32_VVAR_ADDR;
    ADDR vvar_sz = arch_sz == 64 ? LINUX_64_VVAR_SZ : LINUX_32_VVAR_SZ;

    arion->mem->unmap(vvar_load_addr, vvar_load_addr + vvar_sz);
    return arion->mem->map(vvar_load_addr, vvar_sz, LINUX_VVAR_PERMS, "[vvar]");
}

ADDR ElfLoader::map_vdso()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->arch->get_attrs()->arch_sz;

    ADDR vdso_load_addr = arch_sz == 64 ? LINUX_64_VDSO_ADDR : LINUX_32_VDSO_ADDR;
    ADDR vdso_sz = arch_sz == 64 ? LINUX_64_VDSO_SZ : LINUX_32_VDSO_SZ;

    arion->mem->unmap(vdso_load_addr, vdso_load_addr + vdso_sz);
    ADDR vdso_addr = arion->mem->map(vdso_load_addr, vdso_sz, LINUX_VDSO_PERMS, "[vdso]");

    if (arion->arch->get_attrs()->arch == CPU_ARCH::X86_ARCH)
    {
        size_t bin_vdso_sz = _binary_vdso_bin_end - _binary_vdso_bin_start;
        arion->mem->write(vdso_addr, (BYTE *)_binary_vdso_bin_start, bin_vdso_sz);
    }

    return vdso_addr;
}

ADDR ElfLoader::map_vsyscall()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    if (arion->arch->get_attrs()->arch != CPU_ARCH::X8664_ARCH) // For now, only x86-64 implements vsyscall segment
        return 0;

    std::array<std::string, 3> vsyscalls = {"gettimeofday", "time", "getcpu"};

    size_t vsyscall_seg_sz = vsyscalls.size() * VSYSCALL_ENTRY_SZ;
    vsyscall_seg_sz += LINUX_64_VSYSCALL_ALIGN - (vsyscall_seg_sz % LINUX_64_VSYSCALL_ALIGN);

    arion->mem->unmap(LINUX_64_VSYSCALL_ADDR, LINUX_64_VSYSCALL_ADDR + vsyscall_seg_sz);
    ADDR vsyscall_addr = arion->mem->map(LINUX_64_VSYSCALL_ADDR, vsyscall_seg_sz, LINUX_VSYSCALL_PERMS, "[vsyscall]");

    for (size_t syscall_i = 0; syscall_i < vsyscalls.size(); syscall_i++)
    {
        std::string syscall_name = vsyscalls.at(syscall_i);
        uint64_t syscall_no = arion->arch->get_syscall_no_by_name(syscall_name);
        std::array<BYTE, VSYSCALL_ENTRY_SZ> syscall_asm =
            static_cast<ArchManagerX8664 *>(arion->arch.get())->gen_vsyscall_entry(syscall_no);
        arion->mem->write(vsyscall_addr + syscall_i * VSYSCALL_ENTRY_SZ, syscall_asm.data(),
                          syscall_asm.size() * sizeof(BYTE));
    }
    return 0;
}

ADDR ElfLoader::map_arm_traps()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::vector<ARM_TRAP> traps;
    traps.push_back({std::string("memory_barrier"), 0xfa0,
                     std::vector<BYTE>{0xba, 0x0f, 0x07, 0xee, 0x00, 0xf0, 0x20, 0xe3, 0x0e, 0xf0, 0xa0, 0xe1}});
    traps.push_back(
        {std::string("cmpxchg"), 0xfc0, std::vector<BYTE>{0x00, 0x30, 0x92, 0xe5, 0x00, 0x30, 0x53, 0xe0, 0x00, 0x10,
                                                          0x82, 0x05, 0x00, 0x00, 0x73, 0xe2, 0x0e, 0xf0, 0xa0, 0xe1}});
    traps.push_back(
        {std::string("get_tls"), 0xfe0,
         std::vector<BYTE>{0x08, 0x00, 0x9f, 0xe5, 0x0e, 0xf0, 0xa0, 0xe1, 0x70, 0x0f, 0x1d, 0xee, 0xe7, 0xfd,
                           0xde, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}});

    ADDR arm_traps_addr =
        arion->mem->map(LINUX_32_ARM_TRAPS_ADDR, LINUX_32_ARM_TRAPS_SIZE, LINUX_ARM_TRAPS_PERMS, "[arm_traps]");

    for (ARM_TRAP trap : traps)
        arion->mem->write(arm_traps_addr + trap.off, trap.code.data(), trap.code.size() * sizeof(BYTE));
    return 0;
}

void ElfLoader::setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto prog_elf_attrs = std::dynamic_pointer_cast<ARION_ELF_PARSER_ATTRIBUTES>(prog_parser->get_attrs());

    this->write_auxv_entry(AUXV::AT_ENTRY,
                           this->is_pie ? prog_elf_attrs->entry + params->load_address : prog_elf_attrs->entry);
    this->write_auxv_entry(AUXV::AT_BASE, (!this->is_static)
                                              ? (arch_sz == 64 ? LINUX_64_INTERP_ADDR : LINUX_32_INTERP_ADDR)
                                              : params->load_address);
    this->write_auxv_entry(AUXV::AT_PHNUM, prog_elf_attrs->prog_headers_n);
    this->write_auxv_entry(AUXV::AT_PHENT, prog_elf_attrs->prog_headers_entry_sz);
    this->write_auxv_entry(AUXV::AT_PHDR, params->load_address + prog_elf_attrs->prog_headers_off);
    if (arion->arch->get_attrs()->arch == CPU_ARCH::X86_ARCH)
        this->write_auxv_entry(AUXV::AT_SYSINFO, params->vdso_address + LINUX_VDSO_KERNEL_VSYSCALL_OFF);
    if (arion->arch->get_attrs()->seg_flags & ARION_VDSO_PRESENT)
        this->write_auxv_entry(AUXV::AT_SYSINFO_EHDR, params->vdso_address);
}
