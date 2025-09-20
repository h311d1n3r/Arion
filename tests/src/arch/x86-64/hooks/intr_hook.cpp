#include <arion/arion.hpp>
#include <arion_test/common.hpp>

using namespace arion;

bool intr_hook_bkpt_hit = false;

void intr_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (intno == 3)
        intr_hook_bkpt_hit = true;
}

TEST_F(ArionTest, IntrHook)
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::LOG_LEVEL>("log_lvl", arion::LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::string rootfs_path = this->arion_root_path + "/rootfs/x86-64/rootfs";
    std::shared_ptr<Arion> arion = Arion::new_instance({rootfs_path + "/root/simple_syscalls/simple_syscalls"},
                                                       rootfs_path, {}, rootfs_path + "/root", std::move(config));
    arion->hooks->hook_intr(intr_hook);
    arion::REG pc_reg = arion->arch->get_attrs()->regs.pc;
    arion::RVAL64 pc = arion->arch->read_arch_reg(pc_reg);
    arion->mem->write_val(pc, 0xCC, 1); // Breakpoint
    arion_group->add_arion_instance(arion);
    try
    {
        arion_group->run();
    }
    catch (std::exception e)
    {
    }
    EXPECT_TRUE(intr_hook_bkpt_hit);
}
