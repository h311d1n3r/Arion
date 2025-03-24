#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>
#include <filesystem>

using namespace arion;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <binary>" << std::endl;
        return 1;
    }
    std::unique_ptr<CONFIG> config = std::make_unique<CONFIG>();
    config->log_lvl = ARION_LOG_LEVEL::DEBUG;
    config->enable_sleep_syscall = true;
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    std::shared_ptr<Arion> arion = Arion::new_instance({argv[1]}, "/", {}, std::filesystem::current_path(), std::move(config));
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->run();
    return 0;
}
