#include <arion/arion.hpp>
#include <arion/platforms/linux/archs/lnx_arch_x86.hpp>

std::unique_ptr<std::map<arion::REG, arion::RVAL>> ArchManagerLinuxX86::prstatus_to_regs(
    std::vector<arion::BYTE> prstatus)
{
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> regs = std::make_unique<std::map<arion::REG, arion::RVAL>>();

    return std::move(regs);
}
