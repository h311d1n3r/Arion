// This implementation was inspired from Sascha Schirra's blogpost :
// https://scoding.de/setting-global-descriptor-table-unicorn

#include <arion/archs/x86/gdt_manager.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/unicorn/x86.h>
#include <memory>

using namespace arion;
using namespace arion_exception;

std::unique_ptr<GdtManager> GdtManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::move(std::make_unique<GdtManager>(arion));
}

uint16_t GdtManager::setup_selector(uint8_t idx, uint16_t flags)
{
    uint16_t selector = flags;
    selector |= idx << 3;
    return selector;
}

uint64_t GdtManager::setup_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    uint64_t entry = (uint64_t)limit & 0xFFFF;
    entry |= ((uint64_t)base & 0xffffff) << 16;
    entry |= ((uint64_t)access & 0xff) << 40;
    entry |= (((uint64_t)limit >> 16) & 0xf) << 48;
    entry |= ((uint64_t)flags & 0xff) << 52;
    entry |= (((uint64_t)base >> 24) & 0xff) << 56;
    return entry;
}

void GdtManager::setup()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    this->gdt_addr = arion->mem->map(ARION_GDT_ADDR, ARION_GDT_LIMIT, ARION_GDT_PERMS, "[GDT]");
    uint64_t *gdt_entries = (uint64_t *)malloc(ARION_GDT_ENTRY_SZ * ARION_GDT_ENTRIES_N);
    memset(gdt_entries, 0, ARION_GDT_ENTRY_SZ * ARION_GDT_ENTRIES_N);

    ADDR gs_addr = arion->mem->map(ARION_GS_SEGMENT_ADDR, ARION_GS_SEGMENT_SZ, ARION_GS_SEGMENT_PERMS, "[GS]");
    gdt_entries[15] = this->setup_entry(
        gs_addr, ARION_GS_SEGMENT_SZ,
        ARION_A_PRESENT | ARION_A_DATA | ARION_A_DATA_WRITABLE | ARION_A_PRIV_3 | ARION_A_DIR_CON_BIT, ARION_F_PROT_32);
    gdt_entries[16] = this->setup_entry(
        0, 0xfffff000, ARION_A_PRESENT | ARION_A_DATA | ARION_A_DATA_WRITABLE | ARION_A_PRIV_3 | ARION_A_DIR_CON_BIT,
        ARION_F_PROT_32);
    gdt_entries[17] = this->setup_entry(0, 0xfffff000,
                                        ARION_A_PRESENT | ARION_A_CODE | ARION_A_CODE_READABLE | ARION_A_EXEC |
                                            ARION_A_PRIV_3 | ARION_A_DIR_CON_BIT,
                                        ARION_F_PROT_32);
    gdt_entries[18] = this->setup_entry(
        0, 0xfffff000, ARION_A_PRESENT | ARION_A_DATA | ARION_A_DATA_WRITABLE | ARION_A_PRIV_0 | ARION_A_DIR_CON_BIT,
        ARION_F_PROT_32);

    arion->mem->write(this->gdt_addr, (BYTE *)gdt_entries, ARION_GDT_ENTRY_SZ * ARION_GDT_ENTRIES_N);
    free(gdt_entries);

    uc_x86_mmr gdtr_val = {0, this->gdt_addr, ARION_GDT_ENTRY_SZ * ARION_GDT_ENTRIES_N - 1, 0};
    std::array<BYTE, 32> gdtr_arr{};
    memcpy(gdtr_arr.data(), &gdtr_val, sizeof(uc_x86_mmr));
    arion->arch->write_reg<RVAL256>(UC_X86_REG_GDTR, gdtr_arr);

    arion->arch->write_reg<RVAL16>(UC_X86_REG_GS, this->setup_selector(15, ARION_S_GDT | ARION_S_PRIV_3));
    arion->arch->write_reg<RVAL16>(UC_X86_REG_DS, this->setup_selector(16, ARION_S_GDT | ARION_S_PRIV_3));
    arion->arch->write_reg<RVAL16>(UC_X86_REG_CS, this->setup_selector(17, ARION_S_GDT | ARION_S_PRIV_3));
    arion->arch->write_reg<RVAL16>(UC_X86_REG_SS, this->setup_selector(18, ARION_S_GDT | ARION_S_PRIV_0));
}

uint8_t GdtManager::find_free_idx(uint8_t start_idx)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint8_t idx = start_idx;
    while (idx < ARION_GDT_ENTRIES_N)
    {
        uint64_t entry = arion->mem->read_val(this->gdt_addr + idx * ARION_GDT_ENTRY_SZ, ARION_GDT_ENTRY_SZ);
        if (!entry)
            return idx;
        idx++;
    }
    throw NoFreeGdtEntryException();
}

uint32_t GdtManager::get_segment_base(uint16_t selector)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uint8_t idx = selector >> 3;
    uint64_t entry = arion->mem->read_val(this->gdt_addr + idx * ARION_GDT_ENTRY_SZ, ARION_GDT_ENTRY_SZ);

    uint32_t base = (entry >> 16) & 0xFFFFFF;
    base |= ((entry >> 56) & 0xFF) << 24;

    return base;
}

void GdtManager::insert_entry(uint8_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (limit > (1 << 16))
    {
        limit >>= 12;
        flags |= ARION_F_GRANULARITY;
    }
    uint64_t entry = this->setup_entry(base, limit, access, flags);
    arion->mem->write_val(this->gdt_addr + idx * ARION_GDT_ENTRY_SZ, entry, ARION_GDT_ENTRY_SZ);
}
