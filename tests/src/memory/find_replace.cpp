#include <arion/arion.hpp>
#include <arion_test/common.hpp>
#include <filesystem>

char find_replace_flag[] = "A simple print";
char find_replace_flag2[] = "REPLACED";

TEST_F(ArionTest, FindReplace)
{
    testing::internal::CaptureStdout();
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion = Arion::new_instance({this->targets_path+"/simple_print/build/simple_print"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    for (std::shared_ptr<ARION_MAPPING> mapping : arion->mem->get_mappings())
    {
        if (mapping->perms & 4)
        {
            std::vector<arion::BYTE> data =
                    arion->mem->read(mapping->start_addr, mapping->end_addr - mapping->start_addr);
            auto flag_it = std::search(data.begin(), data.end(), find_replace_flag, find_replace_flag + sizeof(find_replace_flag) - 1);
            if (flag_it == data.end())
                continue;
            arion::ADDR flag_addr = mapping->start_addr + std::distance(data.begin(), flag_it);
            std::string mem_flag = arion->mem->read_c_string(flag_addr);
            EXPECT_STREQ(mem_flag.c_str(), find_replace_flag);
            arion->mem->write_string(flag_addr, find_replace_flag2);
            std::string mem_flag2 = arion->mem->read_c_string(flag_addr);
            EXPECT_STREQ(mem_flag2.c_str(), find_replace_flag2);
        }
    }
    arion_group->run();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_STREQ(output.c_str(), "REPLACED\n");
}
