#include <arion/arion.hpp>
#include <filesystem>

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    group->add_arion_instance(arion);
    group->run();
}
