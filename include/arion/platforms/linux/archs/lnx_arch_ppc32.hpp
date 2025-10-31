#ifndef ARION_LNX_ARCH_PPC32_HPP
#define ARION_LNX_ARCH_PPC32_HPP

#include <arion/archs/arch_ppc32.hpp>
#include <arion/platforms/linux/lnx_arch_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <vector>

namespace arion_lnx_ppc32
{

struct pt_regs
{
    arion_lnx_type::elf_greg_t gpr[32];
    arion_lnx_type::elf_greg_t nip;
    arion_lnx_type::elf_greg_t msr;
    arion_lnx_type::elf_greg_t orig_gpr3;
    arion_lnx_type::elf_greg_t ctr;
    arion_lnx_type::elf_greg_t link;
    arion_lnx_type::elf_greg_t xer;
    arion_lnx_type::elf_greg_t ccr;
    arion_lnx_type::elf_greg_t mq;
    arion_lnx_type::elf_greg_t trap;
    arion_lnx_type::elf_greg_t dar;
    arion_lnx_type::elf_greg_t dsisr;
    arion_lnx_type::elf_greg_t result;
    arion_lnx_type::elf_greg_t pad[4];
};

inline std::vector<arion::REG> uc_pt_regs = {
    UC_PPC_REG_0,  UC_PPC_REG_1,  UC_PPC_REG_2,   UC_PPC_REG_3,   UC_PPC_REG_4,   UC_PPC_REG_5,
    UC_PPC_REG_6,  UC_PPC_REG_7,  UC_PPC_REG_8,   UC_PPC_REG_9,   UC_PPC_REG_10,  UC_PPC_REG_11,
    UC_PPC_REG_12, UC_PPC_REG_13, UC_PPC_REG_14,  UC_PPC_REG_15,  UC_PPC_REG_16,  UC_PPC_REG_17,
    UC_PPC_REG_18, UC_PPC_REG_19, UC_PPC_REG_20,  UC_PPC_REG_21,  UC_PPC_REG_22,  UC_PPC_REG_23,
    UC_PPC_REG_24, UC_PPC_REG_25, UC_PPC_REG_26,  UC_PPC_REG_27,  UC_PPC_REG_28,  UC_PPC_REG_29,
    UC_PPC_REG_30, UC_PPC_REG_31, UC_PPC_REG_PC,  UC_PPC_REG_MSR, UC_PPC_REG_INVALID, UC_PPC_REG_CTR,
    UC_PPC_REG_LR, UC_PPC_REG_XER, UC_PPC_REG_CR, UC_PPC_REG_INVALID, UC_PPC_REG_INVALID, UC_PPC_REG_INVALID,
    UC_PPC_REG_INVALID, UC_PPC_REG_INVALID, UC_PPC_REG_INVALID, UC_PPC_REG_INVALID, UC_PPC_REG_INVALID, UC_PPC_REG_INVALID};

const size_t ELF_NGREG = (sizeof(struct pt_regs) / sizeof(arion_lnx_type::elf_greg_t));
typedef arion_lnx_type::elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_prstatus
{
    struct arion_lnx_type::elf_prstatus_common common;
    elf_gregset_t pr_reg;
    int pr_fpvalid;
};

struct fp_regset
{
    double fpregs[32];
    double fpscr;
};

typedef struct fp_regset elf_fpregset_t;

inline std::vector<arion::REG> uc_fp_regs = {
    UC_PPC_REG_FPR0,  UC_PPC_REG_FPR1,  UC_PPC_REG_FPR2,  UC_PPC_REG_FPR3,  UC_PPC_REG_FPR4,  UC_PPC_REG_FPR5,
    UC_PPC_REG_FPR6,  UC_PPC_REG_FPR7,  UC_PPC_REG_FPR8,  UC_PPC_REG_FPR9,  UC_PPC_REG_FPR10, UC_PPC_REG_FPR11,
    UC_PPC_REG_FPR12, UC_PPC_REG_FPR13, UC_PPC_REG_FPR14, UC_PPC_REG_FPR15, UC_PPC_REG_FPR16, UC_PPC_REG_FPR17,
    UC_PPC_REG_FPR18, UC_PPC_REG_FPR19, UC_PPC_REG_FPR20, UC_PPC_REG_FPR21, UC_PPC_REG_FPR22, UC_PPC_REG_FPR23,
    UC_PPC_REG_FPR24, UC_PPC_REG_FPR25, UC_PPC_REG_FPR26, UC_PPC_REG_FPR27, UC_PPC_REG_FPR28, UC_PPC_REG_FPR29,
    UC_PPC_REG_FPR30, UC_PPC_REG_FPR31, UC_PPC_REG_FPSCR};

class ArchManagerLinuxPPC32 : public arion_ppc32::ArchManagerPPC32, public arion::LinuxArchManager
{
  private:
    std::map<arion::REG, arion::RVAL> prstatus_to_regs(std::vector<arion::BYTE> prstatus) override;
    std::map<arion::REG, arion::RVAL> fpregset_to_regs(std::vector<arion::BYTE> fpregset) override;
};

}; // namespace arion_lnx_ppc32

#endif // ARION_LNX_ARCH_PPC32_HPP