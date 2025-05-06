#include <arion/archs/abi_arm.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/unicorn/arm.h>

using namespace arion;

std::array<arion::BYTE, VSYSCALL_ENTRY_SZ> AbiManagerARM::gen_vsyscall_entry(uint64_t syscall_no)
{
    return std::array<arion::BYTE, VSYSCALL_ENTRY_SZ>();
}

void AbiManagerARM::int_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (intno == 0x2)
        arion->syscalls->process_syscall(arion);
}

void AbiManagerARM::set_thumb_state(uint32_t entrypoint) {
    if ((entrypoint & 1) == 1) {
        this->is_thumb = 1;
    }
    else {
        this->is_thumb = 0;
    }
}

bool AbiManagerARM::get_thumb_mode() {
    return this->is_thumb;
}

ks_engine *AbiManagerARM::curr_ks()
{
    return this->ks.at(this->is_thumb ? ARION_THUMB_MODE : ARION_ARM_MODE);
}

csh *AbiManagerARM::curr_cs()
{
    return this->cs.at(this->is_thumb ? ARION_THUMB_MODE : ARION_ARM_MODE);
}

void AbiManagerARM::enable_vfp()
{
    uc_arm_cp_reg cpacr = {0};
    cpacr.cp = 15;
    cpacr.is64 = 0;
    cpacr.sec = 0;
    cpacr.crn = 1;
    cpacr.crm = 0;
    cpacr.opc1 = 0;
    cpacr.opc2 = 2;
    uc_err uc_reg_err = uc_reg_read(this->uc, UC_ARM_REG_CP_REG, &cpacr); // CPACR
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegReadException(uc_reg_err);

    cpacr.val |= (0b11 << 20) | (0b11 << 22);

    uc_reg_err = uc_reg_write(this->uc, UC_ARM_REG_CP_REG, &cpacr);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);

    uint32_t fpexc = 0;
    uc_reg_err = uc_reg_read(this->uc, UC_ARM_REG_FPEXC, &fpexc);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegReadException(uc_reg_err);

    fpexc |= 1 << 30;

    uc_reg_err = uc_reg_write(this->uc, UC_ARM_REG_FPEXC, &fpexc);
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);
}

void AbiManagerARM::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    arion->hooks->hook_intr(AbiManagerARM::int_hook);
    this->enable_vfp();
    this->is_thumb = 0;
}
