#include <arion/platforms/linux/lnx_arch_manager.hpp>

using namespace arion;

std::unique_ptr<PARSED_COREDUMP_THREAD> LinuxArchManager::parse_coredump_thread(
    std::shared_ptr<Arion> arion, std::unique_ptr<ELF_COREDUMP_THREAD> thread,
    std::shared_ptr<ElfParser> prog_parser)
{
    std::unique_ptr<PARSED_COREDUMP_THREAD> parsed_thread = std::make_unique<PARSED_COREDUMP_THREAD>();
    auto prog_elf_attrs = std::dynamic_pointer_cast<ELF_PARSER_ATTRIBUTES>(prog_parser->get_attrs());

    std::map<arion::REG, arion::RVAL> regs;
    if (thread->raw_prstatus.size())
    {
        std::map<arion::REG, arion::RVAL> gp_regs =
            this->prstatus_to_regs(thread->raw_prstatus); // General-purpose registers
        regs.insert(gp_regs.begin(), gp_regs.end());
    }
    if (thread->raw_fpregset.size())
    {
        std::map<arion::REG, arion::RVAL> fp_regs =
            this->fpregset_to_regs(thread->raw_fpregset); // Floating-point registers
        regs.insert(fp_regs.begin(), fp_regs.end());
    }

    parsed_thread->thread = std::move(thread);
    parsed_thread->regs = std::move(regs);
    return parsed_thread;
}
