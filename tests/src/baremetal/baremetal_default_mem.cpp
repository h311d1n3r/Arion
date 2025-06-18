#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <arion_test/shellcode/basic_shellcode.hpp>

TEST_P(ArionMultiarchTest, BaremetalDefaultMem)
{
    testing::internal::CaptureStdout();
    try
    {
        std::unique_ptr<Config> config = std::make_unique<Config>();
        config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
        std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
        std::string rootfs_path = this->arion_root_path + "/rootfs/" + this->arch + "/rootfs";
        auto arch_it = arion::ARCH_FROM_NAME.find(str_to_uppercase(this->arch));
        if(arch_it == arion::ARCH_FROM_NAME.end())
            FAIL() << "No architecture with name : " << this->arch;
        auto code_it = BASIC_SHELLCODES.find(this->arch);
        if(code_it == BASIC_SHELLCODES.end())
            FAIL() << "No shellcode for architecture : " << this->arch;
        std::unique_ptr<BaremetalManager> baremetal = std::make_unique<BaremetalManager>(arch_it->second, code_it->second, 0x400000);
        std::shared_ptr<Arion> arion = Arion::new_instance(std::move(baremetal),
                                                           rootfs_path, {}, rootfs_path + "/root", std::move(config));
        arion_group->add_arion_instance(arion);
        arion_group->run();
    }
    catch (std::exception& e)
    {
        testing::internal::GetCapturedStdout();
        FAIL() << "Exception caught: " << e.what();
    }
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_STREQ(output.c_str(), "Hello World!\n");
}
