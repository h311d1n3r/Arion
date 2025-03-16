#ifndef ARION_GDT_MANAGER_HPP
#define ARION_GDT_MANAGER_HPP

#include <arion/common/global_defs.hpp>

#define ARION_GDT_ADDR 0xC0000000
#define ARION_GDT_LIMIT 0x1000
#define ARION_GDT_ENTRY_SZ 0x8
#define ARION_GDT_ENTRIES_N 0x20
#define ARION_GDT_PERMS 0x6

#define ARION_GS_SEGMENT_ADDR 0xC0001000
#define ARION_GS_SEGMENT_SZ 0x1000
#define ARION_GS_SEGMENT_PERMS 0x6

#define ARION_F_GRANULARITY 0x8
#define ARION_F_PROT_32 0x4
#define ARION_F_LONG 0x2
#define ARION_F_AVAILABLE 0x1
#define ARION_A_PRESENT 0x80
#define ARION_A_PRIV_3 0x60
#define ARION_A_PRIV_2 0x40
#define ARION_A_PRIV_1 0x20
#define ARION_A_PRIV_0 0x0
#define ARION_A_CODE 0x10
#define ARION_A_DATA 0x10
#define ARION_A_TSS 0x0
#define ARION_A_GATE 0x0
#define ARION_A_EXEC 0x8
#define ARION_A_DATA_WRITABLE 0x2
#define ARION_A_CODE_READABLE 0x2
#define ARION_A_DIR_CON_BIT 0x4
#define ARION_S_GDT 0x0
#define ARION_S_LDT 0x4
#define ARION_S_PRIV_3 0x3
#define ARION_S_PRIV_2 0x2
#define ARION_S_PRIV_1 0x1
#define ARION_S_PRIV_0 0x0

#include <cstdint>
#include <memory>

class Arion;

class GdtManager
{
  private:
    arion::ADDR gdt_addr;
    std::weak_ptr<Arion> arion;
    uint16_t setup_selector(uint8_t idx, uint16_t flags);
    uint64_t setup_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);

  public:
    static std::unique_ptr<GdtManager> initialize(std::weak_ptr<Arion> arion);
    GdtManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    void setup();
    uint8_t find_free_idx(uint8_t start_idx);
    void insert_entry(uint8_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
};

#endif // ARION_GDT_MANAGER_HPP
