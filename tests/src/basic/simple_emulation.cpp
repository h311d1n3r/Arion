#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <filesystem>

TEST_F(ArionTest, SimpleEmulation)
{
    testing::internal::CaptureStdout();
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion = Arion::new_instance({this->targets_path+"/simple_print/build/simple_print"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    arion_group->run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_STREQ(output.c_str(), "A simple print\n");
}
