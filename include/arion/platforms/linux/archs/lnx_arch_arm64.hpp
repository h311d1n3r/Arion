#ifndef ARION_LNX_ARCH_ARM64_HPP
#define ARION_LNX_ARCH_ARM64_HPP

#include <arion/archs/arch_arm64.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>

namespace arion_lnx_arm64
{
struct user_pt_regs
{
    __u64 regs[31];
    __u64 sp;
    __u64 pc;
    __u64 pstate;
};

struct frame_record
{
    uint64_t fp;
    uint64_t lr;
};

struct frame_record_meta
{
    struct frame_record record;
    uint64_t type;
};

struct pt_regs
{
    union {
        struct user_pt_regs user_regs;
        struct
        {
            uint64_t regs[31];
            uint64_t sp;
            uint64_t pc;
            uint64_t pstate;
        };
    };
    uint64_t orig_x0;
    int32_t syscallno;
    uint32_t pmr;

    uint64_t sdei_ttbr1;
    struct frame_record_meta stackframe;

    uint64_t lockdep_hardirqs;
    uint64_t exit_rcu;
};

const size_t ELF_NGREG = (sizeof(struct user_pt_regs) / sizeof(elf_greg_t));
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};
} // namespace arion_lnx_arm64

class ArchManagerLinuxARM64 : public ArchManagerARM64, public LinuxArchManager
{
  public:
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
};

#endif // ARION_LNX_ARCH_ARM64_HPP
