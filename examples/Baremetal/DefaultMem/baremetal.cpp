#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/baremetal_manager.hpp>
#include <arion/unicorn/x86.h>
#include <arion/platforms/linux/lnx_baremetal_loader.hpp>
#include <iostream>
#include <memory>
#include <filesystem>

using namespace arion;

std::vector<arion::BYTE> BASIC_SHELLCODE_X86_64 = {
    0x48,0x83,0xec,0x10,0xc7,0x04,0x24,0x00,0x00,0x00,0x48,
    0xc7,0x44,0x24,0x04,0x65,0x6c,0x6c,0x6f,0xc7,0x44,0x24,
    0x08,0x20,0x57,0x6f,0x72,0xc7,0x44,0x24,0x0c,0x6c,0x64,
    0x21,0x0a,0xb8,0x01,0x00,0x00,0x00,0xbf,0x01,0x00,0x00,
    0x00,0x48,0x89,0xe6,0x48,0x83,0xc6,0x03,0xba,0x0e,0x00,
    0x00,0x00,0x0f,0x05,0xb8,0x3c,0x00,0x00,0x00,0x48,0x31,
    0xff,0x0f,0x05
};

void instr_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    std::vector<BYTE> read_data = arion->mem->read(addr, sz);
    cs_insn *insn;
    size_t count = cs_disasm(*arion->abi->curr_cs(), (const uint8_t *)read_data.data(), sz, addr, 0, &insn);
    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
        {
            std::cout << std::hex << insn[i].address << ":\t";
            for (size_t j = 0; j < insn[i].size; j++)
                std::cout << std::hex << std::setw(2) << std::setfill('0') << +insn[i].bytes[j] << " ";
            if (insn[i].size < 8)
                std::cout << std::string((8 - insn[i].size) * 3, ' ');
            std::cout << "\t" << insn[i].mnemonic << "\t" << insn[i].op_str << std::endl;
        }

        cs_free(insn, count);
    }
    else
    {
        std::cerr << "Failed to disassemble code at 0x" << std::hex << addr << std::endl;
    }
}

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::DEBUG);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::unique_ptr<BaremetalManager> baremetal = std::make_unique<BaremetalManager>(CPU_ARCH::X8664_ARCH, BASIC_SHELLCODE_X86_64, 0x400000);
    // Arion::new_instance(baremetal, fs_root, env, cwd, log_level, config)
    std::shared_ptr<Arion> arion = Arion::new_instance(std::move(baremetal), "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    arion_group->run();
    return 0;
}
