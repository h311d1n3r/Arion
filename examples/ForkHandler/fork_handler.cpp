#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>
#include <filesystem>

using namespace arion;

void execve_hook(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> new_inst, void *user_data)
{
    std::cout << "EXECVE !" << std::endl;
    std::cout << "PID : 0x" << std::hex << +new_inst->get_pid() << std::endl;
    std::cout << "PROCESS IMAGE : " << new_inst->get_program_args().at(0) << std::endl;
    // Here you can apply hooks on the new_inst instance
}

void fork_hook(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> child, void *user_data)
{
    std::cout << "FORK !" << std::endl;
    std::cout << "PARENT PID : 0x" << std::hex << +arion->get_pid() << std::endl;
    std::cout << "CHILD PID : 0x" << std::hex << +child->get_pid() << std::endl;
    // Here you can apply hooks on the child instance
    child->hooks->hook_execve(execve_hook);
}

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<ARION_LOG_LEVEL>("log_lvl", ARION_LOG_LEVEL::DEBUG);
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    std::shared_ptr<Arion> arion = Arion::new_instance(std::vector<std::string>{"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->hooks->hook_execve(execve_hook);
    arion->hooks->hook_fork(fork_hook);
    arion->run();
    return 0;
}
