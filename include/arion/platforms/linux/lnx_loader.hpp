#ifndef ARION_LNX_LOADER_HPP
#define ARION_LNX_LOADER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <cstdint>
#include <memory>
#include <string>

#define LINUX_64_LOAD_ADDR 0x400000
#define LINUX_64_STACK_ADDR 0x7ffffffde000
#define LINUX_64_STACK_SZ 0x21000

#define LINUX_32_LOAD_ADDR 0x8040000
#define LINUX_32_STACK_ADDR 0xfffcf000
#define LINUX_32_STACK_SZ 0x21000

#define LINUX_STACK_PERMS 6

namespace arion
{

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

struct AUXV_PTRS
{
    ADDR random_addr;
    ADDR prog_name_addr;
    ADDR platform_name_addr;
};

struct ARION_EXPORT LNX_LOADER_PARAMS
{
    ADDR load_address;
    ADDR interp_address;
    ADDR vvar_address;
    ADDR vdso_address;
    ADDR stack_address;
    ADDR vsyscall_address;
    ADDR arm_traps_address;
};

class LinuxLoader
{
  private:
    void setup_auxv(std::unique_ptr<AUXV_PTRS> auxv_ptrs, std::shared_ptr<LNX_LOADER_PARAMS> params);
    void setup_envp(std::vector<ADDR> envp_ptrs);
    void setup_argv(std::vector<ADDR> argv_ptrs);

  protected:
    std::weak_ptr<Arion> arion;
    const std::vector<std::string> program_args;
    const std::vector<std::string> program_env;
    LinuxLoader(std::weak_ptr<Arion> arion, const std::vector<std::string> program_args,
                const std::vector<std::string> program_env)
        : arion(arion), program_args(program_args), program_env(program_env) {};
    void write_auxv_entry(AUXV auxv, uint64_t val);
    virtual void setup_specific_auxv(std::shared_ptr<LNX_LOADER_PARAMS> params, uint16_t arch_sz) = 0;
    ADDR map_stack(std::shared_ptr<LNX_LOADER_PARAMS> params, std::string path);
    void init_main_thread(std::shared_ptr<LNX_LOADER_PARAMS> params, ADDR entry_addr);

  public:
    virtual std::unique_ptr<LNX_LOADER_PARAMS> process() = 0;
};

}; // namespace arion

#endif // ARION_LNX_LOADER_HPP
