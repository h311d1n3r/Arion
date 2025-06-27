#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/archs/abi_x86-64.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/platforms/linux/lnx_baremetal_loader.hpp>
#include <cstdint>
#include <memory>

using namespace arion;

std::unique_ptr<LNX_LOADER_PARAMS> LinuxBaremetalLoader::process()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<LNX_LOADER_PARAMS> params = std::make_shared<LNX_LOADER_PARAMS>();
    std::vector<BYTE> code = arion->baremetal->get_code();
    if(code.size())
        params->load_address = this->map_code(code);
    
    KERNEL_SEG_FLAGS seg_flags = arion->abi->get_attrs()->seg_flags;
    if (seg_flags & ARION_VVAR_PRESENT && arion->baremetal->additional_mapped_segments.VVAR)
        params->vvar_address = this->map_vvar();
    else
        params->vvar_address = 0;
    if (seg_flags & ARION_VDSO_PRESENT  && arion->baremetal->additional_mapped_segments.VDSO)
        params->vdso_address = this->map_vdso();
    else
        params->vdso_address = 0;
    if (arion->baremetal->additional_mapped_segments.STACK) {
        params->stack_address = this->map_stack(params, "[baremetal]");
    }
    if (seg_flags & ARION_VSYSCALL_PRESENT && arion->baremetal->additional_mapped_segments.VSYSCALL)
        params->vsyscall_address = this->map_vsyscall();
    else
        params->vsyscall_address = 0;
    if (seg_flags & ARION_ARM_TRAPS_PRESENT && arion->baremetal->additional_mapped_segments.ARM_TRAPS)
        params->arm_traps_address = this->map_arm_traps();
    else
        params->arm_traps_address = 0;

    this->init_main_thread(params, arion->baremetal->get_entry_addr());
    return std::make_unique<LNX_LOADER_PARAMS>(*params.get());
}

ADDR LinuxBaremetalLoader::map_code(std::vector<BYTE> code) {
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    size_t code_sz = code.size();
    size_t mapping_sz = code_sz;
    if(mapping_sz % LINUX_BAREMETAL_CODE_ALIGN)
        mapping_sz += LINUX_BAREMETAL_CODE_ALIGN - (mapping_sz % LINUX_BAREMETAL_CODE_ALIGN);
    ADDR load_addr = arion->mem->map(arion->baremetal->get_load_addr(), mapping_sz, LINUX_BAREMETAL_CODE_PERMS, "[code]");
    arion->mem->write(load_addr, code.data(), code.size());
    return load_addr;
}

void LinuxBaremetalLoader::setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) {
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    this->write_auxv_entry(AUXV::AT_ENTRY,arion->baremetal->get_entry_addr());
    this->write_auxv_entry(AUXV::AT_BASE, arion->baremetal->get_load_addr());
}

ADDR LinuxBaremetalLoader::map_vvar()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->abi->get_attrs()->arch_sz;

    ADDR vvar_load_addr = arch_sz == 64 ? LINUX_64_VVAR_ADDR : LINUX_32_VVAR_ADDR;
    ADDR vvar_sz = arch_sz == 64 ? LINUX_64_VVAR_SZ : LINUX_32_VVAR_SZ;

    return arion->mem->map(vvar_load_addr, vvar_sz, LINUX_VVAR_PERMS, "[vvar]");
}

ADDR LinuxBaremetalLoader::map_vdso()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->abi->get_attrs()->arch_sz;

    ADDR vdso_load_addr = arch_sz == 64 ? LINUX_64_VDSO_ADDR : LINUX_32_VDSO_ADDR;
    ADDR vdso_sz = arch_sz == 64 ? LINUX_64_VDSO_SZ : LINUX_32_VDSO_SZ;

    ADDR vdso_addr = arion->mem->map(vdso_load_addr, vdso_sz, LINUX_VDSO_PERMS, "[vdso]");

    if (arion->abi->get_attrs()->arch == CPU_ARCH::X86_ARCH)
    {
        size_t bin_vdso_sz = _binary_vdso_bin_end - _binary_vdso_bin_start;
        arion->mem->write(vdso_addr, (BYTE *)_binary_vdso_bin_start, bin_vdso_sz);
    }

    return vdso_addr;
}

ADDR LinuxBaremetalLoader::map_vsyscall()
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

ADDR LinuxBaremetalLoader::map_arm_traps()
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