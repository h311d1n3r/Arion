#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>

using namespace arion;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <binary>" << std::endl;
        return 1;
    }
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "Error getting current working directory" << std::endl;
        return 1;
    }
    std::shared_ptr<Arion> arion = Arion::new_instance({argv[1]}, "/", {}, cwd, ARION_LOG_LEVEL::DEBUG);
    arion->config->enable_sleep_syscall = true;
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->run();
    return 0;
}
