#include <arion/arion.hpp>
#include <filesystem>
#include <iostream>

void fork_hook(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> child, void *user_data)
{
    std::cout << "FORK" << std::endl;
    std::cout << "PARENT PID : " << std::hex << +arion->get_pid() << std::endl;
    std::cout << "CHILD PID : " << std::hex << +child->get_pid() << std::endl;
}

int main() {
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<Arion> arion = Arion::new_instance({"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion->hooks->hook_fork(fork_hook);
    arion->run();
}