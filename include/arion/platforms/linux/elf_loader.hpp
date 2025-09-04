#ifndef ARION_ELF_LOADER_HPP
#define ARION_ELF_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_loader.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_64_VVAR_ADDR 0x7ffff7fba000
#define LINUX_64_VVAR_SZ 0x4000
#define LINUX_64_VDSO_ADDR 0x7ffff7fbe000
#define LINUX_64_VDSO_SZ 0x2000
#define LINUX_64_INTERP_ADDR 0x7ffff7fc0000
#define LINUX_64_VSYSCALL_ADDR 0xffffffffff600000
#define LINUX_64_VSYSCALL_ALIGN 0x1000

#define LINUX_32_VVAR_ADDR 0xf7fba000
#define LINUX_32_VVAR_SZ 0x4000
#define LINUX_32_VDSO_ADDR 0xf7fbe000
#define LINUX_32_VDSO_SZ 0x2000
#define LINUX_32_INTERP_ADDR 0xf7fc0000
#define LINUX_32_ARM_TRAPS_ADDR 0xffff0000
#define LINUX_32_ARM_TRAPS_SIZE 0x1000
#define LINUX_32_ARM_GETTLS_ADDR 0xffff0fe0

#define LINUX_READ_PERMS 4
#define LINUX_WRITE_PERMS 2
#define LINUX_EXEC_PERMS 1

#define LINUX_VVAR_PERMS LINUX_READ_PERMS
#define LINUX_VDSO_PERMS LINUX_READ_PERMS | LINUX_EXEC_PERMS
#define LINUX_VSYSCALL_PERMS LINUX_EXEC_PERMS
#define LINUX_ARM_TRAPS_PERMS LINUX_READ_PERMS | LINUX_EXEC_PERMS

#define HEAP_SZ 0x1000
#define HEAP_PERMS LINUX_READ_PERMS | LINUX_EXEC_PERMS | LINUX_WRITE_PERMS

#define LINUX_VDSO_KERNEL_VSYSCALL_OFF 0x570

struct ARM_TRAP
{
    std::string name;
    off_t off;
    std::vector<arion::BYTE> code;

    ARM_TRAP(std::string name, off_t off, std::vector<arion::BYTE> code)
        : name(std::move(name)), off(off), code(std::move(code)) {};
};

extern char _binary_vdso_bin_start[];
extern char _binary_vdso_bin_end[];

class ElfLoader : LinuxLoader
{
  private:
    std::shared_ptr<ElfParser> prog_parser, interp_parser;
    bool is_pie;
    bool is_static;
    arion::ADDR map_elf_segments(const std::shared_ptr<ElfParser> parser, arion::ADDR load_addr);
    arion::ADDR map_vvar();
    arion::ADDR map_vdso();
    arion::ADDR map_vsyscall();
    arion::ADDR map_arm_traps();
    void init_coredump_threads();

  protected:
    void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) override;

  public:
    ElfLoader(std::weak_ptr<Arion> arion, std::shared_ptr<ElfParser> interp_parser,
              std::shared_ptr<ElfParser> prog_parser, const std::vector<std::string> program_args,
              const std::vector<std::string> program_env)
        : LinuxLoader(arion, program_args, program_env), interp_parser(interp_parser), prog_parser(prog_parser) {};
    ElfLoader(std::weak_ptr<Arion> arion, std::shared_ptr<ElfParser> prog_parser,
              const std::vector<std::string> program_args, const std::vector<std::string> program_env)
        : ElfLoader(arion, nullptr, std::move(prog_parser), program_args, program_env) {};
    std::unique_ptr<LNX_LOADER_PARAMS> process() override;
};

#endif // ARION_ELF_LOADER_HPP
