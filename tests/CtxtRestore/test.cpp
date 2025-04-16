#include <arion/arion.hpp>
#include <filesystem>

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<arion::ARION_LOG_LEVEL>("log_lvl", arion::ARION_LOG_LEVEL::OFF);
    std::shared_ptr<ArionGroup> arion_group = std::make_shared<ArionGroup>();
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    std::shared_ptr<ARION_CONTEXT> ctxt = arion->context->save();
    for (uint8_t i = 0; i < 5; i++)
    {
        arion_group->run();
        arion->context->restore(ctxt);
    }
}
