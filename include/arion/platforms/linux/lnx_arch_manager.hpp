#ifndef ARION_LNX_ARCH_MANAGER_HPP
#define ARION_LNX_ARCH_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <memory>
#include <vector>

struct ARION_PARSED_COREDUMP_THREAD
{
    std::unique_ptr<ARION_ELF_COREDUMP_THREAD> thread;
    std::map<arion::REG, arion::RVAL> regs;

    ARION_PARSED_COREDUMP_THREAD() {};
};

class LinuxArchManager
{
  private:
    virtual std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) = 0;
    virtual std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) = 0;

  public:
    std::unique_ptr<ARION_PARSED_COREDUMP_THREAD> parse_coredump_thread(
        std::shared_ptr<Arion> arion, std::unique_ptr<ARION_ELF_COREDUMP_THREAD> thread,
        std::shared_ptr<ElfParser> prog_parser);
};

#endif // ARION_LNX_ARCH_MANAGER_HPP
