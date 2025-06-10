#ifndef ARION_ELF_LOADER_HPP
#define ARION_ELF_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_64_LOAD_ADDR 0x555555554000
#define LINUX_64_VVAR_ADDR 0x7ffff7fba000
#define LINUX_64_VVAR_SZ 0x4000
#define LINUX_64_VDSO_ADDR 0x7ffff7fbe000
#define LINUX_64_VDSO_SZ 0x2000
#define LINUX_64_INTERP_ADDR 0x7ffff7fc0000
#define LINUX_64_STACK_ADDR 0x7ffffffde000
#define LINUX_64_STACK_SZ 0x21000
#define LINUX_64_VSYSCALL_ADDR 0xffffffffff600000
#define LINUX_64_VSYSCALL_ALIGN 0x1000

#define LINUX_32_LOAD_ADDR 0x8048000
#define LINUX_32_VVAR_ADDR 0xf7fba000
#define LINUX_32_VVAR_SZ 0x4000
#define LINUX_32_VDSO_ADDR 0xf7fbe000
#define LINUX_32_VDSO_SZ 0x2000
#define LINUX_32_INTERP_ADDR 0xf7fc0000
#define LINUX_32_STACK_ADDR 0xfffcf000
#define LINUX_32_STACK_SZ 0x21000
#define LINUX_32_ARM_TRAPS_ADDR 0xffff0000
#define LINUX_32_ARM_TRAPS_SIZE 0x1000
#define LINUX_32_ARM_GETTLS_ADDR 0xffff0fe0

#define LINUX_VVAR_PERMS 4
#define LINUX_VDSO_PERMS 5
#define LINUX_STACK_PERMS 6
#define LINUX_VSYSCALL_PERMS 1
#define LINUX_ARM_TRAPS_PERMS 5

#define HEAP_SZ 0x1000
#define HEAP_PERMS 6

#define LINUX_VDSO_KERNEL_VSYSCALL_OFF 0x570

enum AUXV
{
    AT_NULL = 0,
    AT_IGNORE = 1,
    AT_EXECFD = 2,
    AT_PHDR = 3,
    AT_PHENT = 4,
    AT_PHNUM = 5,
    AT_PAGESZ = 6,
    AT_BASE = 7,
    AT_FLAGS = 8,
    AT_ENTRY = 9,
    AT_NOTELF = 10,
    AT_UID = 11,
    AT_EUID = 12,
    AT_GID = 13,
    AT_EGID = 14,
    AT_PLATFORM = 15,
    AT_HWCAP = 16,
    AT_CLKTCK = 17,
    AT_SECURE = 23,
    AT_BASE_PLATFORM = 24,
    AT_RANDOM = 25,
    AT_HWCAP2 = 26,
    AT_RSEQ_FEATURE_SIZE = 27,
    AT_RSEQ_ALIGN = 28,
    AT_HWCAP3 = 29,
    AT_HWCAP4 = 30,
    AT_EXECFN = 31,
    AT_SYSINFO = 32,
    AT_SYSINFO_EHDR = 33,
    AT_MINSIGSTKSZ = 51
};

struct ARION_EXPORT LOADER_PARAMS
{
    arion::ADDR load_address;
    arion::ADDR interp_address;
    arion::ADDR vvar_address;
    arion::ADDR vdso_address;
    arion::ADDR stack_address;
    arion::ADDR vsyscall_address;
    arion::ADDR arm_traps_address;
};

struct AUXV_PTRS
{
    arion::ADDR random_addr;
    arion::ADDR prog_name_addr;
    arion::ADDR platform_name_addr;
};

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

class ElfLoader
{
  private:
    std::shared_ptr<ElfParser> prog_parser, interp_parser;
    const std::vector<std::string> program_args;
    const std::vector<std::string> program_env;
    std::weak_ptr<Arion> arion;
    bool is_pie;
    bool is_static;
    uint16_t arch_sz;
    arion::ADDR map_elf_segments(const std::shared_ptr<ElfParser> parser, arion::ADDR load_addr);
    arion::ADDR map_vvar();
    arion::ADDR map_vdso();
    void write_auxv_entry(AUXV auxv, uint64_t val);
    void setup_auxv(std::unique_ptr<AUXV_PTRS> auxv_ptrs, std::shared_ptr<LOADER_PARAMS> params);
    void setup_envp(std::vector<arion::ADDR> envp_ptrs);
    void setup_argv(std::vector<arion::ADDR> argv_ptrs);
    arion::ADDR map_stack(std::shared_ptr<LOADER_PARAMS> params);
    arion::ADDR map_vsyscall();
    arion::ADDR map_arm_traps();
    void init_main_thread(std::shared_ptr<LOADER_PARAMS> params);

  public:
    ElfLoader(std::weak_ptr<Arion> arion, std::shared_ptr<ElfParser> interp_parser,
              std::shared_ptr<ElfParser> prog_parser, const std::vector<std::string> program_args,
              const std::vector<std::string> program_env)
        : arion(arion), interp_parser(interp_parser), prog_parser(prog_parser), program_args(program_args),
          program_env(program_env) {};
    ElfLoader(std::weak_ptr<Arion> arion, std::shared_ptr<ElfParser> prog_parser,
              const std::vector<std::string> program_args, const std::vector<std::string> program_env)
        : ElfLoader(arion, nullptr, std::move(prog_parser), program_args, program_env) {};
    std::unique_ptr<LOADER_PARAMS> process();
};

#endif // ARION_ELF_LOADER_HPP
