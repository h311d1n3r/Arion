#ifndef ARION_LNX_BAREMETAL_LOADER_HPP
#define ARION_LNX_BAREMETAL_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <cstdint>
#include <memory>
#include <string>

using namespace arion;

#define LINUX_BAREMETAL_CODE_ALIGN 0x1000

#define LINUX_READ_PERMS 4
#define LINUX_WRITE_PERMS 2
#define LINUX_EXEC_PERMS 1

#define LINUX_VVAR_PERMS LINUX_READ_PERMS
#define LINUX_VDSO_PERMS LINUX_READ_PERMS | LINUX_EXEC_PERMS
#define LINUX_VSYSCALL_PERMS LINUX_EXEC_PERMS
#define LINUX_ARM_TRAPS_PERMS LINUX_READ_PERMS | LINUX_EXEC_PERMS

#define LINUX_BAREMETAL_CODE_PERMS LINUX_READ_PERMS | LINUX_WRITE_PERMS | LINUX_EXEC_PERMS

class LinuxBaremetalLoader : LinuxLoader
{
  private:
    arion::ADDR map_code(std::vector<uint8_t> code);
    arion::ADDR map_vvar();
    arion::ADDR map_vdso();
    arion::ADDR map_vsyscall();
    arion::ADDR map_arm_traps();

  protected:
    void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) override;

  public:
    LinuxBaremetalLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_args, const std::vector<std::string> program_env)
        : LinuxLoader(arion, program_args, program_env) {};
    std::unique_ptr<LNX_LOADER_PARAMS> process() override;
};

#endif // ARION_LNX_BAREMETAL_LOADER_HPP
