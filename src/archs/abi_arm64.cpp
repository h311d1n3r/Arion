#include <arion/archs/abi_arm64.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/unicorn/arm.h>

using namespace arion;

void AbiManagerARM64::int_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (intno == 0x2)
        arion->syscalls->process_syscall(arion);
}

ks_engine *AbiManagerARM64::curr_ks()
{
    return this->ks.at(0);
}

csh *AbiManagerARM64::curr_cs()
{
    return this->cs.at(0);
}

void AbiManagerARM64::enable_lse()
{
    uc_arm64_cp_reg isar0 = {0};
    isar0.crn = 0;
    isar0.crm = 6;
    isar0.op0 = 3;
    isar0.op1 = 0;
    isar0.op2 = 0;
    uc_err uc_reg_err = uc_reg_read(this->uc, UC_ARM64_REG_CP_REG, &isar0); // ID_AA64ISAR0_EL1
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegReadException(uc_reg_err);

    isar0.val |= (0b0010 << 20);

    uc_reg_err = uc_reg_write(this->uc, UC_ARM64_REG_CP_REG, &isar0);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);
}

void AbiManagerARM64::enable_vfp()
{
    uint32_t cpacr = 0;
    uc_err uc_reg_err = uc_reg_read(this->uc, UC_ARM64_REG_CPACR_EL1, &cpacr); // CPACR
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegReadException(uc_reg_err);

    cpacr |= (0b11 << 20) | (0b11 << 22);

    uc_reg_err = uc_reg_write(this->uc, UC_ARM64_REG_CPACR_EL1, &cpacr);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);
}

void AbiManagerARM64::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->hooks->hook_intr(AbiManagerARM64::int_hook);
    this->enable_lse();
    this->enable_vfp();
}

ADDR AbiManagerARM64::dump_tls()
{
    uc_arm64_cp_reg tpidr_el0 = {0};
    tpidr_el0.crn = 13;
    tpidr_el0.crm = 0;
    tpidr_el0.op0 = 3;
    tpidr_el0.op1 = 3;
    tpidr_el0.op2 = 2;

    uc_err uc_reg_err = uc_reg_read(this->uc, UC_ARM64_REG_CP_REG, &tpidr_el0);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);

    return tpidr_el0.val;
}

void AbiManagerARM64::load_tls(ADDR new_tls)
{
    uc_arm64_cp_reg tpidr_el0 = {0};
    tpidr_el0.crn = 13;
    tpidr_el0.crm = 0;
    tpidr_el0.op0 = 3;
    tpidr_el0.op1 = 3;
    tpidr_el0.op2 = 2;
    tpidr_el0.val = new_tls;

    uc_err uc_reg_err = uc_reg_write(this->uc, UC_ARM64_REG_CP_REG, &tpidr_el0);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);
}
