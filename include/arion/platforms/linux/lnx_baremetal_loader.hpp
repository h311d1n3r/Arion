#ifndef ARION_LNX_BAREMETAL_LOADER_HPP
#define ARION_LNX_BAREMETAL_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <cstdint>
#include <memory>
#include <string>

using namespace arion;

#define LINUX_64_STACK_ADDR 0x7ffffffde000
#define LINUX_64_STACK_SZ 0x21000

#define LINUX_32_LOAD_ADDR 0x8040000

#define LINUX_32_STACK_ADDR 0xfffcf000
#define LINUX_32_STACK_SZ 0x21000

#define HEAP_SZ 0x1000
#define DEFAULT_DATA_SIZE 0x1000 * 100

#define LINUX_RWX 1 | 2 | 4
#define LINUX_RW 2 | 4

class LinuxBaremetalLoader
{
  private:
    const std::vector<std::string> program_env;
    std::weak_ptr<Arion> arion;
    ADDR map_default_instance(std::shared_ptr<std::vector<uint8_t>> coderaw, ADDR load_addr);
    void setup_envp(std::vector<ADDR> envp_ptrs);

  public:
    uint16_t arch_sz;
    void ARION_EXPORT init_main_thread(std::shared_ptr<LOADER_PARAMS> params);
    ADDR ARION_EXPORT map_stack(std::shared_ptr<LOADER_PARAMS> params);

    LinuxBaremetalLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_env)
        : arion(arion), program_env(program_env) {};
    ARION_EXPORT LinuxBaremetalLoader(std::weak_ptr<Arion> arion) : arion(arion) {};
    std::unique_ptr<LOADER_PARAMS> process();
};

#endif // ARION_LNX_BAREMETAL_LOADER_HPP
