#include <arion/arion.hpp>
#include <arion_test/common.hpp>

using namespace arion;

char find_replace_flag[] = "A simple print";
char find_replace_flag2[] = "REPLACED";

TEST_P(ArionMultiarchTest, FindReplace)
{
    testing::internal::CaptureStdout();
    try
    {
        std::unique_ptr<Config> config = std::make_unique<Config>();
        config->set_field<arion::LOG_LEVEL>("log_lvl", arion::LOG_LEVEL::OFF);
        std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
        std::string rootfs_path = this->arion_root_path + "/rootfs/" + this->arch + "/rootfs";
        std::shared_ptr<Arion> arion = Arion::new_instance({rootfs_path + "/root/simple_print/simple_print"},
                                                           rootfs_path, {}, rootfs_path + "/root", std::move(config));
        arion_group->add_arion_instance(arion);
        for (std::shared_ptr<ARION_MAPPING> mapping : arion->mem->get_mappings())
        {
            if (mapping->perms & 4)
            {
                std::vector<arion::BYTE> data =
                    arion->mem->read(mapping->start_addr, mapping->end_addr - mapping->start_addr);
                auto flag_it = std::search(data.begin(), data.end(), find_replace_flag,
                                           find_replace_flag + sizeof(find_replace_flag) - 1);
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
    }
    catch (std::exception e)
    {
        testing::internal::GetCapturedStdout(); // Prevent using GetCapturedStdout() multiple times
        FAIL() << "Exception caught: " << e.what();
    }
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find(find_replace_flag2), std::string::npos);
}
