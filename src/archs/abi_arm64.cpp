#include <arion/archs/abi_arm64.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <unicorn/arm.h>

using namespace arion;

std::array<arion::BYTE, VSYSCALL_ENTRY_SZ> AbiManagerARM64::gen_vsyscall_entry(uint64_t syscall_no)
{
    return std::array<arion::BYTE, VSYSCALL_ENTRY_SZ>();
}

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
