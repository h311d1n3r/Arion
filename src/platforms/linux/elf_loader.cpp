#include <arion/archs/abi_x86-64.hpp>
#include <arion/arion.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/unicorn/unicorn.h>
#include <arion/utils/fs_utils.hpp>
#include <arion/utils/math_utils.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <unistd.h>

using namespace arion;

std::unique_ptr<LOADER_PARAMS> ElfLoader::process()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<LOADER_PARAMS> params = std::make_shared<LOADER_PARAMS>();
    this->is_pie = prog_parser->type == ELF_FILE_TYPE::EXEC;
    this->is_static = !!this->interp_parser;
    this->arch_sz = arion->abi->get_attrs()->arch_sz;
    params->load_address = this->map_elf_segments(
        this->prog_parser, this->is_pie ? 0 : (this->arch_sz == 64 ? LINUX_64_LOAD_ADDR : LINUX_32_LOAD_ADDR));
    if (this->is_static)
        params->interp_address = this->map_elf_segments(
            this->interp_parser, this->arch_sz == 64 ? LINUX_64_INTERP_ADDR : LINUX_32_INTERP_ADDR);

    KERNEL_SEG_FLAGS seg_flags = arion->abi->get_attrs()->seg_flags;
    if (seg_flags & ARION_VVAR_PRESENT)
        params->vvar_address = this->map_vvar();
    else
        params->vvar_address = 0;
    if (seg_flags & ARION_VDSO_PRESENT)
        params->vdso_address = this->map_vdso();
    else
        params->vdso_address = 0;
    params->stack_address = this->map_stack(params);
    if (seg_flags & ARION_VSYSCALL_PRESENT)
        params->vsyscall_address = this->map_vsyscall();
    else
        params->vsyscall_address = 0;
    if (seg_flags & ARION_ARM_TRAPS_PRESENT)
        params->arm_traps_address = this->map_arm_traps();
    else
        params->arm_traps_address = 0;
    this->init_main_thread(params);
    REG pc = arion->abi->get_attrs()->regs.pc;
    return std::make_unique<LOADER_PARAMS>(*params.get());
}

ADDR ElfLoader::map_elf_segments(const std::shared_ptr<ElfParser> parser, ADDR load_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    const std::string program_name = parser->elf_path;
    ADDR first_addr = ARION_MAX_U64, last_addr = 0;
    for (const std::unique_ptr<struct SEGMENT> &seg : parser->segments)
    {
        ADDR seg_data_start_addr = seg->virt_addr + load_addr;
        ADDR seg_start_addr = seg_data_start_addr;
        ADDR seg_end_addr = seg_start_addr + seg->virt_sz;
        seg_start_addr -= (seg_start_addr % seg->align);
        seg_end_addr += (seg->align - (seg_end_addr % seg->align));
        size_t data_seg_sz = seg_end_addr - seg_data_start_addr;
        if (seg->phy_sz > data_seg_sz)
            seg_end_addr += seg->align;
        size_t seg_sz = seg_end_addr - seg_start_addr;
        if (seg_end_addr > last_addr)
            last_addr = seg_end_addr;
        if (seg_start_addr < first_addr)
            first_addr = seg_start_addr;

        arion->mem->map(seg_start_addr, seg_sz, seg->flags, program_name);

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

    ADDR vvar_load_addr = this->arch_sz == 64 ? LINUX_64_VVAR_ADDR : LINUX_32_VVAR_ADDR;
    ADDR vvar_sz = this->arch_sz == 64 ? LINUX_64_VVAR_SZ : LINUX_32_VVAR_SZ;

    return arion->mem->map(vvar_load_addr, vvar_sz, LINUX_VVAR_PERMS, "[vvar]");
}

ADDR ElfLoader::map_vdso()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR vdso_load_addr = this->arch_sz == 64 ? LINUX_64_VDSO_ADDR : LINUX_32_VDSO_ADDR;
    ADDR vdso_sz = this->arch_sz == 64 ? LINUX_64_VDSO_SZ : LINUX_32_VDSO_SZ;

    ADDR vdso_addr = arion->mem->map(vdso_load_addr, vdso_sz, LINUX_VDSO_PERMS, "[vdso]");

    if (arion->abi->get_attrs()->arch == CPU_ARCH::X86_ARCH)
    {
        size_t bin_vdso_sz = _binary_vdso_bin_end - _binary_vdso_bin_start;
        arion->mem->write(vdso_addr, (BYTE *)_binary_vdso_bin_start, bin_vdso_sz);
    }

    return vdso_addr;
}

void ElfLoader::write_auxv_entry(AUXV auxv, uint64_t val)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(val);
    arion->mem->stack_push(auxv);
}

void ElfLoader::setup_auxv(std::unique_ptr<AUXV_PTRS> auxv_ptrs, std::shared_ptr<LOADER_PARAMS> params)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    this->write_auxv_entry(AUXV::AT_NULL, 0);
    this->write_auxv_entry(AUXV::AT_RSEQ_ALIGN, 0x20);
    this->write_auxv_entry(AUXV::AT_RSEQ_FEATURE_SIZE, 0x1c);
    this->write_auxv_entry(AUXV::AT_PLATFORM, auxv_ptrs->platform_name_addr);
    this->write_auxv_entry(AUXV::AT_EXECFN, auxv_ptrs->prog_name_addr);
    this->write_auxv_entry(AUXV::AT_HWCAP2, arion->abi->get_attrs()->hwcap2);
    this->write_auxv_entry(AUXV::AT_RANDOM, auxv_ptrs->random_addr);
    this->write_auxv_entry(AUXV::AT_SECURE, 0);
    this->write_auxv_entry(AUXV::AT_EGID, getegid());
    this->write_auxv_entry(AUXV::AT_GID, getgid());
    this->write_auxv_entry(AUXV::AT_EUID, geteuid());
    this->write_auxv_entry(AUXV::AT_UID, getuid());
    this->write_auxv_entry(AUXV::AT_ENTRY,
                           this->is_pie ? this->prog_parser->entry : this->prog_parser->entry + params->load_address);
    this->write_auxv_entry(AUXV::AT_FLAGS, 0);
    this->write_auxv_entry(AUXV::AT_BASE, this->is_static
                                              ? (this->arch_sz == 64 ? LINUX_64_INTERP_ADDR : LINUX_32_INTERP_ADDR)
                                              : params->load_address);
    this->write_auxv_entry(AUXV::AT_PHNUM, this->prog_parser->prog_headers_n);
    this->write_auxv_entry(AUXV::AT_PHENT, this->prog_parser->prog_headers_entry_sz);
    this->write_auxv_entry(AUXV::AT_PHDR, params->load_address + this->prog_parser->prog_headers_off);
    this->write_auxv_entry(AUXV::AT_CLKTCK, 0x100);
    this->write_auxv_entry(AUXV::AT_PAGESZ, ARION_SYSTEM_PAGE_SZ);
    this->write_auxv_entry(AUXV::AT_HWCAP, arion->abi->get_attrs()->hwcap);
    this->write_auxv_entry(AUXV::AT_MINSIGSTKSZ, 0xe30);
    if (arion->abi->get_attrs()->arch == CPU_ARCH::X86_ARCH)
        this->write_auxv_entry(AUXV::AT_SYSINFO, params->vdso_address + LINUX_VDSO_KERNEL_VSYSCALL_OFF);
    if (arion->abi->get_attrs()->seg_flags & ARION_VDSO_PRESENT)
        this->write_auxv_entry(AUXV::AT_SYSINFO_EHDR, params->vdso_address);
}

void ElfLoader::setup_envp(std::vector<ADDR> envp_ptrs)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(0);
    for (auto envp_ptr_it = envp_ptrs.rbegin(); envp_ptr_it != envp_ptrs.rend(); ++envp_ptr_it)
        arion->mem->stack_push(*envp_ptr_it);
}

void ElfLoader::setup_argv(std::vector<ADDR> argv_ptrs)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(0);
    for (auto argv_ptr_it = argv_ptrs.rbegin(); argv_ptr_it != argv_ptrs.rend(); ++argv_ptr_it)
        arion->mem->stack_push(*argv_ptr_it);
    arion->mem->stack_push(argv_ptrs.size());
}

ADDR ElfLoader::map_stack(std::shared_ptr<LOADER_PARAMS> params)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR stack_load_addr = this->arch_sz == 64 ? LINUX_64_STACK_ADDR : LINUX_32_STACK_ADDR;
    ADDR stack_sz = this->arch_sz == 64 ? LINUX_64_STACK_SZ : LINUX_32_STACK_SZ;
    REG sp_reg = arion->abi->get_attrs()->regs.sp;

    arion->mem->map(stack_load_addr, stack_sz, LINUX_STACK_PERMS, "[stack]");
    arion->abi->write_reg(sp_reg, stack_load_addr + stack_sz);

    std::unique_ptr<AUXV_PTRS> auxv_ptrs = std::make_unique<AUXV_PTRS>();
    arion->mem->stack_push_string(arion->abi->get_attrs()->arch_name);
    auxv_ptrs->platform_name_addr = arion->abi->read_arch_reg(sp_reg);
    arion->mem->stack_push_string(this->prog_parser->elf_path);
    auxv_ptrs->prog_name_addr = arion->abi->read_arch_reg(sp_reg);
    std::vector<BYTE> ran_bytes = gen_random_bytes(16);
    arion->mem->stack_push_bytes(ran_bytes.data(), ran_bytes.size());
    auxv_ptrs->random_addr = arion->abi->read_arch_reg(sp_reg);

    std::vector<ADDR> envp_ptrs;
    for (std::string env : this->program_env)
    {
        arion->mem->stack_push_string(env);
        envp_ptrs.push_back(arion->abi->read_arch_reg(sp_reg));
    }

    std::vector<ADDR> argv_ptrs;
    for (std::string arg : this->program_args)
    {
        arion->mem->stack_push_string(arg);
        argv_ptrs.push_back(arion->abi->read_arch_reg(sp_reg));
    }

    arion->mem->stack_align();

    this->setup_auxv(std::move(auxv_ptrs), std::move(params));
    this->setup_envp(std::move(envp_ptrs));
    this->setup_argv(std::move(argv_ptrs));

    return stack_load_addr;
}

ADDR ElfLoader::map_vsyscall()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    if (arion->abi->get_attrs()->arch != CPU_ARCH::X8664_ARCH) // For now, only x86-64 implements vsyscall segment
        return 0;

    std::array<std::string, 3> vsyscalls = {"gettimeofday", "time", "getcpu"};

    size_t vsyscall_seg_sz = vsyscalls.size() * VSYSCALL_ENTRY_SZ;
    vsyscall_seg_sz += LINUX_64_VSYSCALL_ALIGN - (vsyscall_seg_sz % LINUX_64_VSYSCALL_ALIGN);

    ADDR vsyscall_addr = arion->mem->map(LINUX_64_VSYSCALL_ADDR, vsyscall_seg_sz, LINUX_VSYSCALL_PERMS, "[vsyscall]");

    for (size_t syscall_i = 0; syscall_i < vsyscalls.size(); syscall_i++)
    {
        std::string syscall_name = vsyscalls.at(syscall_i);
        uint64_t syscall_no = arion->abi->get_syscall_no_by_name(syscall_name);
        std::array<BYTE, VSYSCALL_ENTRY_SZ> syscall_asm =
            static_cast<AbiManagerX8664 *>(arion->abi.get())->gen_vsyscall_entry(syscall_no);
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

void ElfLoader::init_main_thread(std::shared_ptr<LOADER_PARAMS> params)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<ARION_MAPPING> stack_mapping = arion->mem->get_mapping_at(params->stack_address);
    REG sp = arion->abi->get_attrs()->regs.sp;

    ADDR entry_addr;
    if (this->is_static)
        entry_addr = this->interp_parser->entry + params->interp_address;
    else
        entry_addr = this->is_pie ? this->prog_parser->entry : this->prog_parser->entry + params->load_address;

    ADDR sp_val = arion->abi->read_arch_reg(sp);
    std::unique_ptr<std::map<REG, RVAL>> regs = arion->abi->init_thread_regs(entry_addr, sp_val);
    std::unique_ptr<ARION_THREAD> arion_t = std::make_unique<ARION_THREAD>(0, 0, 0, 0, std::move(regs), 0);
    arion->abi->load_regs(std::move(arion_t->regs_state));
    if (arion->abi->get_attrs()->arch == CPU_ARCH::ARM_ARCH) {
        arion->abi->set_thumb_state(entry_addr);
    }
    arion->threads->add_thread_entry(std::move(arion_t));
}
