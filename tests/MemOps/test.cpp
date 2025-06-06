#include <arion/arion.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

char flag[] = "FIND_ME_IN_MEMORY";
char flag2[] = "REPLACED";

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    for (std::shared_ptr<ARION_MAPPING> mapping : arion->mem->get_mappings())
    {
        if (mapping->perms & 4)
        {
            std::vector<arion::BYTE> data =
                arion->mem->read(mapping->start_addr, mapping->end_addr - mapping->start_addr);
            auto flag_it = std::search(data.begin(), data.end(), flag, flag + sizeof(flag) - 1);
            if (flag_it == data.end())
                continue;
            std::cout << "Found flag in memory." << std::endl;
            arion::ADDR flag_addr = mapping->start_addr + std::distance(data.begin(), flag_it);
            std::string mem_flag = arion->mem->read_c_string(flag_addr);
            std::cout << "Flag is : " << mem_flag << std::endl;
            arion->mem->write_string(flag_addr, flag2);
            std::string mem_flag2 = arion->mem->read_c_string(flag_addr);
            std::cout << "Flag2 is : " << mem_flag2 << std::endl;
        }
    }
    arion_group->run();
}
