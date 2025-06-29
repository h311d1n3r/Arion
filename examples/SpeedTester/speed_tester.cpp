#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <arion/unicorn/x86.h>
#include <arion/utils/convert_utils.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <time.h>

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
    std::shared_ptr<ARION_CONTEXT> ctxt = arion->context->save();
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint16_t i = 0; i < 100; i++)
    {
        arion_group->run();
        arion->context->restore(ctxt);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    std::cout << "100 executions of /bin/ls in " << +elapsed << " seconds";
    return 0;
}
