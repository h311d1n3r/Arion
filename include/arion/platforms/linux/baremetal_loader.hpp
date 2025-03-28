#ifndef ARION_BAREMETAL_LOADER_HPP
#define ARION_BAREMETAL_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <cstdint>
#include <memory>
#include <string>

using namespace arion;

#define LINUX_64_STACK_ADDR 0x7ffffffde000
#define LINUX_64_STACK_SZ 0x21000

#define LINUX_32_LOAD_ADDR 0x8048000

#define LINUX_32_STACK_ADDR 0xfffcf000
#define LINUX_32_STACK_SZ 0x21000

#define HEAP_SZ 0x1000
#define DEFAULT_DATA_SIZE 0x1000 * 100

#define PROT_READ  1
#define PROT_EXEC  2
#define PROT_WRITE 4

#define LINUX_STACK_PERMS 6

class BaremetalLoader
{
  private:
    const std::vector<std::string> program_env;
    std::weak_ptr<Arion> arion;
    uint16_t arch_sz;
    ADDR map_default_instance(std::shared_ptr<std::vector<uint8_t>> coderaw, ADDR load_addr);
    ADDR map_stack(std::shared_ptr<LOADER_PARAMS> params);
    void init_main_thread(std::shared_ptr<LOADER_PARAMS> params);
    void setup_envp(std::vector<ADDR> envp_ptrs);

  public:
    BaremetalLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_env)
        : arion(arion), program_env(program_env) {};
    std::unique_ptr<LOADER_PARAMS> process();
};

#endif // ARION_BAREMETAL_LOADER_HPP
