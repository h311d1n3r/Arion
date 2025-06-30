#include <arion/archs/arch_arm.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/unicorn/arm.h>

using namespace arion;

void ArchManagerARM::int_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (intno == 0x2)
        arion->syscalls->process_syscall(arion);
}

bool ArchManagerARM::is_thumb()
{
    RVAL32 cpsr = this->read_reg<RVAL32>(UC_ARM_REG_CPSR);
    return cpsr & ARION_ARM_CPSR_THUMB_BIT;
}

ks_engine *ArchManagerARM::curr_ks()
{
    return this->ks.at(this->is_thumb() ? ARION_THUMB_MODE : ARION_ARM_MODE);
}

csh *ArchManagerARM::curr_cs()
{
    return this->cs.at(this->is_thumb() ? ARION_THUMB_MODE : ARION_ARM_MODE);
}

void ArchManagerARM::enable_vfp()
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

void ArchManagerARM::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->hooks->hook_intr(ArchManagerARM::int_hook);
    this->enable_vfp();
}

ADDR ArchManagerARM::dump_tls()
{
    uc_arm_cp_reg cp15 = {0};
    cp15.cp = 15;
    cp15.is64 = 0;
    cp15.sec = 0;
    cp15.crn = 13;
    cp15.crm = 0;
    cp15.opc1 = 0;
    cp15.opc2 = 3;

    uc_err uc_reg_err = uc_reg_read(this->uc, UC_ARM_REG_CP_REG, &cp15); // TPIDRURO
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);

    return cp15.val;
}

void ArchManagerARM::load_tls(ADDR new_tls)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uc_arm_cp_reg cp15 = {0};
    cp15.cp = 15;
    cp15.is64 = 0;
    cp15.sec = 0;
    cp15.crn = 13;
    cp15.crm = 0;
    cp15.opc1 = 0;
    cp15.opc2 = 3;
    cp15.val = new_tls;

    uc_err uc_reg_err = uc_reg_write(this->uc, UC_ARM_REG_CP_REG, &cp15); // TPIDRURO
    if (uc_reg_err != UC_ERR_OK)
        throw UnicornRegWriteException(uc_reg_err);

    arion->mem->write_ptr(LINUX_32_ARM_GETTLS_ADDR + 0x10, new_tls);
}

void ArchManagerARM::prerun_hook(arion::ADDR &start)
{
    if (this->is_thumb())
        start |= 1;
}
