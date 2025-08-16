#ifndef ARION_LNX_ARCH_MANAGER_HPP
#define ARION_LNX_ARCH_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <memory>
#include <vector>

class LinuxArchManager
{
  public:
    virtual std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) = 0;
};

#endif // ARION_LNX_ARCH_MANAGER_HPP
