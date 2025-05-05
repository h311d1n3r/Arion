#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <filesystem>

TEST_F(ArionTest, SimpleForks)
{
    testing::internal::CaptureStdout();
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion = Arion::new_instance({this->targets_path+"/multi_fork/build/multi_fork"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    arion_group->run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_STREQ(output.c_str(), "2\n2\n3\n3\n4\n4\n");
}
