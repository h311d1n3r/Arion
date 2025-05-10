#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <filesystem>

size_t insn_hook_syscall_ctr = 0;

void insn_hook(std::shared_ptr<Arion> arion, void *user_data)
{
    insn_hook_syscall_ctr++;
}

TEST_F(ArionTest, InsnHook)
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion = Arion::new_instance({this->targets_path+"/simple_syscalls/build/simple_syscalls"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion->hooks->hook_insn(insn_hook, UC_X86_INS_SYSCALL);
    arion_group->add_arion_instance(arion);
    arion_group->run();
    EXPECT_GE(insn_hook_syscall_ctr, 3);
}
