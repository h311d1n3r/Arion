#include <arion/archs/arch_x86.hpp>
#include <arion/arion.hpp>
#include <asm/ldt.h>

using namespace arion;
using namespace arion_x86;
using namespace arion_exception;

ks_engine *ArchManagerX86::curr_ks()
{
    return this->ks.at(0);
}

csh *ArchManagerX86::curr_cs()
{
    return this->cs.at(0);
}

void ArchManagerX86::int_hook(std::shared_ptr<Arion> arion, uint32_t intno, void *user_data)
{
    if (intno == 0x80)
        arion->syscalls->process_syscall(arion);
}

void ArchManagerX86::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->hooks->hook_intr(ArchManagerX86::int_hook);
    // May implement sysenter hook by the future instead of patching vdso.bin to remove that instruction
}

ADDR ArchManagerX86::dump_tls()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    RVAL16 tls_selector = arion->arch->read_reg<RVAL16>(UC_X86_REG_GS);
    if (!tls_selector)
        return 0;

    return tls_selector;
}

uint16_t ArchManagerX86::new_tls(ADDR udesc_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    struct user_desc *u_desc = (struct user_desc *)malloc(sizeof(struct user_desc));
    std::vector<BYTE> u_desc_data = arion->mem->read(udesc_addr, sizeof(struct user_desc));
    memcpy(u_desc, u_desc_data.data(), u_desc_data.size());

    if (u_desc->entry_number == 0xFFFFFFFF)
        u_desc->entry_number = 12;
    u_desc->entry_number = arion->gdt_manager->find_free_idx(u_desc->entry_number);
    arion->gdt_manager->insert_entry(
        u_desc->entry_number, u_desc->base_addr, u_desc->limit,
        ARION_A_PRESENT | ARION_A_DATA | ARION_A_DATA_WRITABLE | ARION_A_PRIV_3 | ARION_A_DIR_CON_BIT, ARION_F_PROT_32);
    if (!arion->mem->is_mapped(u_desc->base_addr))
        arion->mem->map(u_desc->base_addr,
                        u_desc->limit_in_pages ? (u_desc->limit * ARION_SYSTEM_PAGE_SZ) : u_desc->limit, 0x6, "[TLS]");
    uint16_t selector = arion->gdt_manager->setup_selector(u_desc->entry_number, ARION_S_GDT | ARION_S_PRIV_3);

    arion->mem->write(udesc_addr, (BYTE *)u_desc, sizeof(struct user_desc));
    free(u_desc);

    return selector;
}

void ArchManagerX86::load_tls(ADDR new_tls)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion->arch->write_reg<RVAL16>(UC_X86_REG_GS, new_tls);
}
