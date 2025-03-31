#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/baremetal.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/baremetal_loader.hpp>
#include <iostream>
#include <memory>
#include <filesystem>

using namespace arion;

unsigned char shellcode[] = {
    0xb8, 0x3c, 0x00, 0x00, 0x00,  // mov eax, 60
    0x48, 0x31, 0xff,              // xor rdi, rdi
    0x0f, 0x05                     // syscall
};

using namespace arion;

void instr_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    std::vector<BYTE> read_data = arion->mem->read(addr, sz);
    cs_insn *insn;
    size_t count = cs_disasm(*arion->abi->curr_cs(), (const uint8_t *)read_data.data(), sz, addr, 0, &insn);
    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            std::cout << "PID: 0x" << std::hex << +arion->get_pid() << " TID: 0x" << std::hex
                      << +arion->threads->get_running_tid() << " - " << insn[i].address << ":\t" << insn[i].mnemonic
                      << "\t" << insn[i].op_str << std::endl;
        }

        cs_free(insn, count);
    }
    else
    {
        std::cerr << "Failed to disassemble code." << std::endl;
    }
}

void block_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    ADDR rsp = arion->abi->read_reg<RVAL64>("RSP");
    uint64_t curr_stack_val = arion->mem->read_val(rsp, sizeof(uint64_t));
    std::cout << "New basic block at 0x" << std::hex << +addr << " RSP : 0x" << std::hex << +rsp << " [RSP] : 0x"
              << std::hex << +curr_stack_val << std::endl;
}

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<ARION_LOG_LEVEL>("log_lvl", ARION_LOG_LEVEL::DEBUG);

    std::unique_ptr<Baremetal> baremetal = std::make_unique<Baremetal>();
    baremetal->setup_memory = true; // Tell arion to not create defaults mappings for shellcode

    auto coderaw = baremetal->coderaw;
    coderaw->insert(coderaw->end(), std::begin(shellcode), std::end(shellcode));

    std::vector<std::string> env = {};
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    std::cout << "bitsize bm = " + std::to_string(baremetal->bitsize) << std::endl; //bitsize = 64
    std::shared_ptr<Arion> arion = Arion::new_instance(std::move(baremetal), "/", env, std::filesystem::current_path(), std::move(config));
    std::cout << "bitsize bm = " + std::to_string(arion->shared_from_this()->baremetal->bitsize) << std::endl; //bitsize = junk data
    std::cout << "config = " + std::to_string(arion->config->get_field<arion::ARION_LOG_LEVEL>("log_lvl")) << std::endl; //exception log_lvl not found

    std::shared_ptr<LOADER_PARAMS> params = std::make_shared<LOADER_PARAMS>();
    BaremetalLoader loader(arion->shared_from_this());

    
   


    loader.arch_sz = arion->baremetal->bitsize;
    params->stack_address = loader.map_stack(params);
    loader.init_main_thread(params);
    arion->loader_params = std::make_unique<LOADER_PARAMS>(*params.get());

    std::cout << arion->mem->mappings_str() << std::endl;
    arion->hooks->hook_code(instr_hook);
    arion->hooks->hook_block(block_hook);
    arion->run();
    return 0;
}
