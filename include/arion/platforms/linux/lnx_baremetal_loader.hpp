#ifndef ARION_LNX_BAREMETAL_LOADER_HPP
#define ARION_LNX_BAREMETAL_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_BAREMETAL_CODE_ALIGN 0x1000

#define LINUX_BAREMETAL_CODE_PERMS 0x5

namespace arion
{

class LinuxBaremetalLoader : LinuxLoader
{
  private:
    ADDR map_code(std::vector<uint8_t> code);

  protected:
    void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) override;

  public:
    LinuxBaremetalLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_args,
                         const std::vector<std::string> program_env)
        : LinuxLoader(arion, program_args, program_env) {};
    std::unique_ptr<LNX_LOADER_PARAMS> process() override;
};

}; // namespace arion

#endif // ARION_LNX_BAREMETAL_LOADER_HPP
