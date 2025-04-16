#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

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
    config->set_field<ARION_LOG_LEVEL>("log_lvl", ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    // Arion::new_instance(args, fs_root, env, cwd, log_level, config)
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"/bin/ls"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->hooks->hook_code(instr_hook);
    arion->hooks->hook_block(block_hook);
    arion_group->run();
    return 0;
}
