#include <arion/arion.hpp>
#include <arion_test/common.hpp>

size_t insn_hook_syscall_ctr = 0;

void insn_hook(std::shared_ptr<Arion> arion, void *user_data)
{
    insn_hook_syscall_ctr++;
}

TEST_F(ArionTest, InsnHook)
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::LOG_LEVEL>("log_lvl", arion::LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::string rootfs_path = this->arion_root_path + "/rootfs/x86-64/rootfs";
    std::shared_ptr<Arion> arion = Arion::new_instance({rootfs_path + "/root/simple_syscalls/simple_syscalls"},
                                                       rootfs_path, {}, rootfs_path + "/root", std::move(config));
    arion->hooks->hook_insn(insn_hook, UC_X86_INS_SYSCALL);
    arion_group->add_arion_instance(arion);
    arion_group->run();
    EXPECT_GE(insn_hook_syscall_ctr, 3);
}
