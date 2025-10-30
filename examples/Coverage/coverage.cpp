#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/arion_afl.hpp>
#include <filesystem>
#include <memory>

using namespace arion;

std::shared_ptr<ArionGroup> arion_group;

int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<ARION_LOG_LEVEL>("log_lvl", ARION_LOG_LEVEL::OFF);
    arion_group = std::make_shared<ArionGroup>();
    // Arion::new_instance(args, fs_root, env, cwd, log_level, config)
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    arion->tracer->start("./coverage.drcov", TRACE_MODE::DRCOV);
    arion_group->run();
    arion->tracer->stop();
    return 0;
}
