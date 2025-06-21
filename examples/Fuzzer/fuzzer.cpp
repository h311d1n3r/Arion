#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/components/arion_afl.hpp>
#include <filesystem>
#include <memory>

using namespace arion;

std::shared_ptr<ArionGroup> arion_group;
ADDR read_buf = 0;
size_t read_sz = 0;

bool input_callback(std::shared_ptr<Arion> arion, char *input, size_t input_sz, uint32_t persistent_round,
                    void *user_data)
{
    REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    if (input_sz > read_sz)
        input_sz = read_sz;
    arion->mem->write(read_buf, (BYTE *)input, input_sz);
    arion->abi->write_arch_reg(ret_reg, input_sz);
    return true;
}

bool crash_callback(std::shared_ptr<Arion> arion, uc_err res, char *input, size_t input_sz, uint32_t persistent_round,
                    void *user_data)
{
    return true;
}

void on_syscall_hook(std::shared_ptr<Arion> arion, uint64_t sysno, std::vector<arion::SYS_PARAM> params, bool *handled,
                     void *user_data)
{
    if (sysno == 0)
    {
        uint64_t fd = params.at(0);
        if (fd == 0)
        {
            read_buf = params.at(1);
            read_sz = params.at(2);
            *handled = true;
            arion_group->stop();
        }
    }
}

// Launch the executable with : AFL_AUTORESUME=1 afl-fuzz -i ../corpus -o ./output -U -- ./fuzzer @@
int main()
{
    std::unique_ptr<Config> config = std::make_unique<Config>();
    config->set_field<ARION_LOG_LEVEL>("log_lvl", ARION_LOG_LEVEL::OFF);
    arion_group = std::make_shared<ArionGroup>();
    // Arion::new_instance(args, fs_root, env, cwd, log_level, config)
    std::shared_ptr<Arion> arion =
        Arion::new_instance({"./target"}, "/", {}, std::filesystem::current_path(), std::move(config));
    arion_group->add_arion_instance(arion);
    HOOK_ID hook_sys_id = arion->hooks->hook_syscall(on_syscall_hook);
    arion_group->run();
    arion->hooks->unhook(hook_sys_id);
    ArionAfl afl(arion);
    afl.fuzz(input_callback, crash_callback, {0});
    return 0;
}
