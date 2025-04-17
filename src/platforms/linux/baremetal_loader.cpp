#include <arion/arion.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/utils/fs_utils.hpp>
#include <arion/utils/math_utils.hpp>
#include <arion/platforms/linux/baremetal_loader.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <arion/unicorn/unicorn.h>
#include <unistd.h>

using namespace arion;

std::unique_ptr<LOADER_PARAMS> BaremetalLoader::process()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<LOADER_PARAMS> params = std::make_shared<LOADER_PARAMS>();
    this->arch_sz = arion->abi->get_attrs()->arch_sz;
    auto coderaw = arion->baremetal->coderaw;
    params->load_address = this->map_default_instance(coderaw, this->arch_sz == 64 ? LINUX_64_LOAD_ADDR : LINUX_32_LOAD_ADDR);
    params->stack_address = this->map_stack(params);
    this->init_main_thread(params);
    return std::make_unique<LOADER_PARAMS>(*params.get());
}

ADDR BaremetalLoader::map_default_instance(std::shared_ptr<std::vector<uint8_t>> coderaw, ADDR load_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->map(load_addr, coderaw->size(), PROT_EXEC | PROT_READ | PROT_WRITE, "[code]");
    arion->mem->write(load_addr, coderaw->data(), coderaw->size());
    auto code_end = arion->mem->align_up(coderaw->size());

    arion->logger->debug("Baremetal Codesize is " + std::to_string(coderaw->size()) + \
                         ". Rounded up to " + std::to_string(code_end));

    arion->mem->map(load_addr + code_end, DEFAULT_DATA_SIZE, PROT_READ | PROT_WRITE, "[data]");
    return load_addr;

}

void BaremetalLoader::setup_envp(std::vector<ADDR> envp_ptrs)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->mem->stack_push(0);
    for (auto envp_ptr_it = envp_ptrs.rbegin(); envp_ptr_it != envp_ptrs.rend(); ++envp_ptr_it)
        arion->mem->stack_push(*envp_ptr_it);
}

ADDR BaremetalLoader::map_stack(std::shared_ptr<LOADER_PARAMS> params)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR stack_load_addr = this->arch_sz == 64 ? LINUX_64_STACK_ADDR : LINUX_32_STACK_ADDR;
    ADDR stack_sz = this->arch_sz == 64 ? LINUX_64_STACK_SZ : LINUX_32_STACK_SZ;
    REG sp_reg = arion->abi->get_attrs()->regs.sp;

    arion->mem->map(stack_load_addr, stack_sz, PROT_READ | PROT_WRITE, "[stack]");
    arion->abi->write_reg(sp_reg, stack_load_addr + stack_sz);

    std::vector<ADDR> envp_ptrs;
    for (std::string env : this->program_env)
    {
        arion->mem->stack_push_string(env);
        envp_ptrs.push_back(arion->abi->read_arch_reg(sp_reg));
    }

    arion->mem->stack_align();

    this->setup_envp(std::move(envp_ptrs));
    return stack_load_addr;
}

void BaremetalLoader::init_main_thread(std::shared_ptr<LOADER_PARAMS> params)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<ARION_MAPPING> stack_mapping = arion->mem->get_mapping_at(params->stack_address);
    REG sp = arion->abi->get_attrs()->regs.sp;

    ADDR entry_addr = params->load_address;

    ADDR sp_val = arion->abi->read_arch_reg(sp);
    std::unique_ptr<std::map<REG, RVAL>> regs = arion->abi->init_thread_regs(entry_addr, sp_val, 0);
    std::unique_ptr<ARION_THREAD> arion_t = std::make_unique<ARION_THREAD>(0, 0, 0, 0, std::move(regs));
    arion->abi->load_regs(std::move(arion_t->regs_state));
    arion->threads->add_thread_entry(std::move(arion_t));
}
