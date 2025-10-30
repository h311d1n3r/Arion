#include <arion/arion.hpp>
#include <arion_test/common.hpp>

using namespace arion;

size_t code_hook_syscall_ctr = 0;

void code_hook(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data)
{
    std::vector<arion::BYTE> read_data = arion->mem->read(addr, sz);
    cs_insn *insn;
    size_t count = cs_disasm(*arion->arch->curr_cs(), (const uint8_t *)read_data.data(), sz, addr, 0, &insn);
    if (count > 0)
    {
        for (size_t i = 0; i < count; i++)
            if (!strcmp(insn[i].mnemonic, "syscall"))
                code_hook_syscall_ctr++;
        cs_free(insn, count);
    }
}

TEST_F(ArionTest, CodeHook)
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::LOG_LEVEL>("log_lvl", arion::LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::string rootfs_path = this->arion_root_path + "/rootfs/x86-64/rootfs";
    std::shared_ptr<Arion> arion = Arion::new_instance({rootfs_path + "/root/simple_syscalls/simple_syscalls"},
                                                       rootfs_path, {}, rootfs_path + "/root", std::move(config));
    arion->hooks->hook_code(code_hook);
    arion_group->add_arion_instance(arion);
    arion_group->run();
    EXPECT_GE(code_hook_syscall_ctr, 3);
}
