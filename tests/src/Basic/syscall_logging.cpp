#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <filesystem>

TEST_F(ArionTest, SyscallLogging)
{
    testing::internal::CaptureStdout();
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::DEBUG);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion = Arion::new_instance({this->targets_path+"/simple_syscalls/build/simple_syscalls"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    arion_group->run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("clock_gettime"), std::string::npos);
    EXPECT_NE(output.find("write"), std::string::npos);
    EXPECT_NE(output.find("exit_group"), std::string::npos);
}
