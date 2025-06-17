#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <arion_test/shellcode.hpp>

TEST_P(ArionMultiarchTest, BaremetalDefaultMappingsHelloWorld)
{
    testing::internal::CaptureStdout();
    try
    {
        unsigned char* selected_shellcode = nullptr;
        size_t shellcode_len = 0;
        std::unique_ptr<Baremetal> baremetal = std::make_unique<Baremetal>();

        if (this->arch == "x86")
        {
            selected_shellcode = shellcode_x86;
            shellcode_len = sizeof(shellcode_x86);
            baremetal->arch = arion::CPU_ARCH::X86_ARCH;
        }
        else if (this->arch == "x86-64")
        {
            selected_shellcode = shellcode_x8664;
            shellcode_len = sizeof(shellcode_x8664);
            baremetal->arch = arion::CPU_ARCH::X8664_ARCH;
        }
        else if (this->arch == "arm")
        {
            selected_shellcode = shellcode_arm;
            shellcode_len = sizeof(shellcode_arm);
            baremetal->arch = arion::CPU_ARCH::ARM_ARCH;
        }
        else if (this->arch == "arm64")
        {
            selected_shellcode = shellcode_arm64;
            shellcode_len = sizeof(shellcode_arm64);
            baremetal->arch = arion::CPU_ARCH::ARM64_ARCH;
        }
        else
        {
            FAIL() << "Unsupported architecture: " << this->arch;
        }

        std::unique_ptr<Config> config = std::make_unique<Config>();
        
        auto coderaw = baremetal->coderaw;
        coderaw->insert(coderaw->end(), selected_shellcode, selected_shellcode + shellcode_len);

        config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);

        std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
        std::string rootfs_path = this->arion_root_path + "/rootfs/" + this->arch + "/rootfs";

        std::shared_ptr<Arion> arion =
            Arion::new_instance(std::move(baremetal), rootfs_path, {}, rootfs_path + "/root", std::move(config));

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
