#include <arion/archs/arch_x86-64.hpp>
#include <arion/arion.hpp>
#include <arion/common/arch_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/lnx_loader.hpp>
#include <arion/unicorn/unicorn.h>
#include <arion/utils/fs_utils.hpp>
#include <arion/utils/math_utils.hpp>
#include <cstdint>
#include <memory>
#include <unistd.h>

using namespace arion;

void LinuxLoader::write_auxv_entry(AUXV auxv, uint64_t val)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(val);
    arion->mem->stack_push(auxv);
}

void LinuxLoader::setup_auxv(std::unique_ptr<AUXV_PTRS> auxv_ptrs, std::shared_ptr<LNX_LOADER_PARAMS> params)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->arch->get_attrs()->arch_sz;

    this->write_auxv_entry(AUXV::AT_NULL, 0);
    this->write_auxv_entry(AUXV::AT_RSEQ_ALIGN, 0x20);
    this->write_auxv_entry(AUXV::AT_RSEQ_FEATURE_SIZE, 0x1c);
    this->write_auxv_entry(AUXV::AT_PLATFORM, auxv_ptrs->platform_name_addr);
    this->write_auxv_entry(AUXV::AT_EXECFN, auxv_ptrs->prog_name_addr);
    this->write_auxv_entry(AUXV::AT_HWCAP2, arion->arch->get_attrs()->hwcap2);
    this->write_auxv_entry(AUXV::AT_RANDOM, auxv_ptrs->random_addr);
    this->write_auxv_entry(AUXV::AT_SECURE, 0);
    this->write_auxv_entry(AUXV::AT_EGID, getegid());
    this->write_auxv_entry(AUXV::AT_GID, getgid());
    this->write_auxv_entry(AUXV::AT_EUID, geteuid());
    this->write_auxv_entry(AUXV::AT_UID, getuid());
    this->write_auxv_entry(AUXV::AT_FLAGS, 0);
    this->write_auxv_entry(AUXV::AT_CLKTCK, 0x100);
    this->write_auxv_entry(AUXV::AT_PAGESZ, ARION_SYSTEM_PAGE_SZ);
    this->write_auxv_entry(AUXV::AT_HWCAP, arion->arch->get_attrs()->hwcap);
    this->write_auxv_entry(AUXV::AT_MINSIGSTKSZ, 0xe30);
    this->setup_specific_auxv(params, arch_sz);
}

void LinuxLoader::setup_envp(std::vector<ADDR> envp_ptrs)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(0);
    for (auto envp_ptr_it = envp_ptrs.rbegin(); envp_ptr_it != envp_ptrs.rend(); ++envp_ptr_it)
        arion->mem->stack_push(*envp_ptr_it);
}

void LinuxLoader::setup_argv(std::vector<ADDR> argv_ptrs)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(0);
    for (auto argv_ptr_it = argv_ptrs.rbegin(); argv_ptr_it != argv_ptrs.rend(); ++argv_ptr_it)
        arion->mem->stack_push(*argv_ptr_it);
    arion->mem->stack_push(argv_ptrs.size());
}

ADDR LinuxLoader::map_stack(std::shared_ptr<LNX_LOADER_PARAMS> params, std::string path)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint16_t arch_sz = arion->arch->get_attrs()->arch_sz;
    std::string arch_name = arion::NAME_FROM_ARCH.at(arion->arch->get_attrs()->arch);

    ADDR stack_load_addr = arch_sz == 64 ? LINUX_64_STACK_ADDR : LINUX_32_STACK_ADDR;
    ADDR stack_sz = arch_sz == 64 ? LINUX_64_STACK_SZ : LINUX_32_STACK_SZ;
    REG sp_reg = arion->arch->get_attrs()->regs.sp;

    arion->mem->map(stack_load_addr, stack_sz, LINUX_STACK_PERMS, "[stack]");
    arion->arch->write_reg(sp_reg, stack_load_addr + stack_sz);

    std::unique_ptr<AUXV_PTRS> auxv_ptrs = std::make_unique<AUXV_PTRS>();
    arion->mem->stack_push_string(arch_name);
    auxv_ptrs->platform_name_addr = arion->arch->read_arch_reg(sp_reg);
    arion->mem->stack_push_string(path);
    auxv_ptrs->prog_name_addr = arion->arch->read_arch_reg(sp_reg);
    std::vector<BYTE> ran_bytes = gen_random_bytes(16);
    arion->mem->stack_push_bytes(ran_bytes.data(), ran_bytes.size());
    auxv_ptrs->random_addr = arion->arch->read_arch_reg(sp_reg);

    std::vector<ADDR> envp_ptrs;
    for (std::string env : this->program_env)
    {
        arion->mem->stack_push_string(env);
        envp_ptrs.push_back(arion->arch->read_arch_reg(sp_reg));
    }

    std::vector<ADDR> argv_ptrs;
    for (std::string arg : this->program_args)
    {
        arion->mem->stack_push_string(arg);
        argv_ptrs.push_back(arion->arch->read_arch_reg(sp_reg));
    }

    arion->mem->stack_align();

    this->setup_auxv(std::move(auxv_ptrs), std::move(params));
    this->setup_envp(std::move(envp_ptrs));
    this->setup_argv(std::move(argv_ptrs));

    return stack_load_addr;
}

void LinuxLoader::init_main_thread(std::shared_ptr<LNX_LOADER_PARAMS> params, ADDR entry_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<ARION_MAPPING> stack_mapping = arion->mem->get_mapping_at(params->stack_address);
    REG sp = arion->arch->get_attrs()->regs.sp;

    ADDR sp_val = arion->arch->read_arch_reg(sp);
    std::unique_ptr<std::map<REG, RVAL>> regs = arion->arch->init_thread_regs(entry_addr, sp_val);
    std::unique_ptr<ARION_THREAD> arion_t = std::make_unique<ARION_THREAD>(0, 0, 0, 0, 0, std::move(regs), 0);
    arion->arch->load_regs(std::move(arion_t->regs_state));
    arion->threads->add_thread_entry(std::move(arion_t));
}
