#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <filesystem>

size_t block_hook_syscall_ctr = 0;

void block_hook(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data)
{
    std::vector<arion::BYTE> read_data = arion->mem->read(addr, sz);
    cs_insn *insn;
    size_t count = cs_disasm(*arion->abi->curr_cs(), (const uint8_t *)read_data.data(), sz, addr, 0, &insn);
    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
            if(!strcmp(insn[i].mnemonic, "syscall")) block_hook_syscall_ctr++;
        cs_free(insn, count);
    }
}

TEST_F(ArionTest, BlockHook)
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion = Arion::new_instance({this->targets_path+"/simple_syscalls/build/simple_syscalls"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion->hooks->hook_block(block_hook);
    arion_group->add_arion_instance(arion);
    arion_group->run();
    EXPECT_GE(block_hook_syscall_ctr, 3);
}
