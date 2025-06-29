#include <arion/arion.hpp>
#include <arion_test/common.hpp>

TEST_P(ArionMultiarchTest, SyscallLogging)
{
    testing::internal::CaptureStdout();
    try
    {
        std::unique_ptr<Config> config = std::make_unique<Config>();
        config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::DEBUG);
        std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
        std::string rootfs_path = this->arion_root_path + "/rootfs/" + this->arch + "/rootfs";
        std::shared_ptr<Arion> arion = Arion::new_instance({rootfs_path + "/root/simple_syscalls/simple_syscalls"},
                                                           rootfs_path, {}, rootfs_path + "/root", std::move(config));
        arion_group->add_arion_instance(arion);
        arion_group->run();
    }
    catch (std::exception e)
    {
        testing::internal::GetCapturedStdout(); // Prevent using GetCapturedStdout() multiple times
        FAIL() << "Exception caught: " << e.what();
    }
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("clock_gettime"), std::string::npos);
    EXPECT_NE(output.find("write"), std::string::npos);
    EXPECT_NE(output.find("exit_group"), std::string::npos);
}
