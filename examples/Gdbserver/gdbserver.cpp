#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <filesystem>
#include <iostream>
#include <memory>

using namespace arion;

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<LOG_LEVEL>("log_lvl", LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    // Arion::new_instance(args, fs_root, env, cwd, log_level, config)
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"/bin/ls"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->run_gdbserver(1337);
    arion_group->run();
    return 0;
}
