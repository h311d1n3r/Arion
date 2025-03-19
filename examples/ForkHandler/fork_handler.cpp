#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <iostream>
#include <memory>

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
    // Arion::new_instance(args, fs_root, env, cwd, log_level)
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "Error getting current working directory" << std::endl;
        return 1;
    }
    std::shared_ptr<Arion> arion = Arion::new_instance({"./target"}, "/", {}, cwd, ARION_LOG_LEVEL::OFF);
    std::cout << arion->mem->mappings_str() << std::endl;
    arion->hooks->hook_execve(execve_hook);
    arion->hooks->hook_fork(fork_hook);
    arion->run();
    return 0;
}
