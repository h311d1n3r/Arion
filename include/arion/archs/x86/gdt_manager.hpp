#ifndef ARION_GDT_MANAGER_HPP
#define ARION_GDT_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <cstdint>
#include <memory>

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

namespace arion
{

class Arion;

/// A class used to setup and interact with the Global Descriptor Table (GDT).
class GdtManager
{
  private:
    /// The address where the GDT stands in memory
    ADDR gdt_addr;
    /// The Arion instance this GdtManager instance is related to.
    std::weak_ptr<Arion> arion;
    /**
     * Generates a segment descriptor to be added to the GDT.
     * @param[in] base The linear address where the segment begins.
     * @param[in] limit The maximum addressable unit.
     * @param[in] access The Access Byte which stores permissions over the segment.
     * @param[in] flags Additional flags.
     * @return The segment descriptor.
     */
    uint64_t setup_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);

  public:
    /**
     * Initializes the GdtManager instance.
     * @param[in] arion The Arion instance this GdtManager instance is related to.
     * @return The initialized GdtManager.
     */
    static std::unique_ptr<GdtManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * A builder for GdtManager instances.
     * @param[in] arion The Arion instance this GdtManager instance is related to.
     */
    GdtManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Used to GDT in memory and its mandatory segments.
     */
    void setup();
    /**
     * Retrieves the first free index in the GDT, where one segment descriptor can then be inserted.
     * @return The index.
     */
    uint8_t find_free_idx(uint8_t start_idx);
    /**
     * Builds a segment selector, identifying the segment descriptor to use.
     * @param[in] idx Index of the selector identifying the descriptor in its table.
     * @param[in] flags Flags related to the selector (table and privileges).
     * @return The selector.
     */
    uint16_t setup_selector(uint8_t idx, uint16_t flags);
    /**
     * Fetches a segment base address from the GDT, based on its corresponding selector.
     * @param[in] selector The selector to identify the GDT segment.
     * @return The base address of the segment.
     */
    uint32_t get_segment_base(uint16_t selector);
    /**
     * Builds and inserts an entry in the GDT, identifying a segment.
     * @param[in] idx Index in the GDT.
     * @param[in] base The linear address where the segment begins.
     * @param[in] limit The maximum addressable unit.
     * @param[in] access The Access Byte which stores permissions over the segment.
     * @param[in] flags Additional flags.
     */
    void insert_entry(uint8_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
};

}; // namespace arion

#endif // ARION_GDT_MANAGER_HPP
