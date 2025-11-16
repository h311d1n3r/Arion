#ifndef ARION_MEMORY_MANAGER_HPP
#define ARION_MEMORY_MANAGER_HPP

#include <arion/capstone/capstone.h>
#include <arion/common/global_defs.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/unicorn/unicorn.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace arion
{

class Arion;

/// This structure holds data associated with a memory mapping.
struct ARION_EXPORT ARION_MAPPING
{
    /// Start memory address of the mapping.
    ADDR start_addr;
    /// End memory address of the mapping.
    ADDR end_addr;
    /// Memory access rights of the mapping.
    PROT_FLAGS perms;
    /// A string describing the mapping.
    std::string info;
    /// This buffer is used to store the mapping data for serialization operations.
    BYTE *saved_data = nullptr;
    /**
     * Builder for ARION_MAPPING instances.
     */
    ARION_MAPPING() {};
    /**
     * Builder for ARION_MAPPING instances.
     * @param[in] start_addr Start memory address of the mapping.
     * @param[in] end_addr End memory address of the mapping.
     * @param[in] perms Memory access rights of the mapping.
     * @param[in] info A string describing the mapping.
     */
    ARION_MAPPING(ADDR start_addr, ADDR end_addr, PROT_FLAGS perms, std::string info = "")
        : start_addr(start_addr), end_addr(end_addr), perms(perms), info(info) {};
    /**
     * Builder used to clone ARION_MAPPING instances.
     * @param[in] arion_m The ARION_MAPPING instance to be cloned.
     */
    ARION_MAPPING(ARION_MAPPING *arion_m)
        : start_addr(arion_m->start_addr), end_addr(arion_m->end_addr), perms(arion_m->perms), info(arion_m->info),
          saved_data(arion_m->saved_data) {};
    /**
     * Destructor for ARION_MAPPING instances.
     */
    ~ARION_MAPPING()
    {
        if (saved_data)
            free(saved_data);
        saved_data = nullptr;
    }
};
/*
 * Serializes an ARION_MAPPING instance into a vector of bytes.
 * @param[in] arion_m The ARION_MAPPING to be serialized.
 * @return The serialized vector of bytes.
 */
std::vector<BYTE> serialize_arion_mapping(ARION_MAPPING *arion_m);
/*
 * Deserializes an ARION_MAPPING instance from a vector of bytes.
 * @param[in] srz_file The serialized vector of bytes.
 * @return The deserialized ARION_MAPPING.
 */
ARION_MAPPING *deserialize_arion_mapping(std::vector<BYTE> srz_mapping);

/// This structure holds data associated with a memory modification event.
struct ARION_MEM_EDIT
{
    /// Start address where memory was changed.
    ADDR addr;
    /// Size of the modified memory.
    size_t sz;

    /**
     * Builder for ARION_MEM_EDIT instances.
     * @param[in] addr Start address where memory was changed.
     * @param[in] sz Size of the modified memory.
     */
    ARION_MEM_EDIT(ADDR addr, size_t sz) : addr(addr), sz(sz) {};
};

/// This class is responsible for recording memory accesses to an Arion instance.
class MemoryRecorder
{
  private:
    /// The Arion instance being recorded.
    std::weak_ptr<Arion> arion;
    /// Recorded memory accesses.
    std::vector<std::shared_ptr<ARION_MEM_EDIT>> edits;
    /// ID of the hook used to record memory writes.
    HOOK_ID write_mem_hook_id;
    /// Indicates whether recording is currently active.
    bool started = false;
    /**
     * The function called when a memory write occurs to the associated Arion instance.
     * @param[in] arion Arion instance that triggered the hook.
     * @param[in] type Type of memory being access.
     * @param[in] addr Memory address where the write occurred.
     * @param[in] size Size of the write in bytes.
     * @param[in] val Value written to memory.
     * @param[in] user_data Optional user-defined data passed to the hook.
     */
    static bool write_mem_hook(std::shared_ptr<Arion> arion, uc_mem_type type, uint64_t addr, int size, int64_t val,
                               void *user_data);
    /**
     * Records a new memory access.
     * @param[in] edit The memory access to record.
     */
    void add_edit(std::shared_ptr<ARION_MEM_EDIT> edit);

  public:
    /**
     * Builder for MemoryRecorder instances.
     * @param[in] arion The Arion instance being recorded.
     */
    MemoryRecorder(std::weak_ptr<Arion> arion) : arion(arion), started(false) {};
    /**
     * Instanciates and initializes new MemoryRecorder objects with some parameters.
     * @param[in] arion The Arion instance being recorded.
     * @return A new Memory recorder instance.
     */
    static std::unique_ptr<MemoryRecorder> initialize(std::weak_ptr<Arion> arion);
    /**
     * Clears all recorded memory accesses.
     */
    void clear();
    /**
     * Starts recording memory accesses.
     */
    void start();
    /**
     * Stops recording memory accesses.
     */
    void stop();
    /**
     * Retrieves all recorded memory accesses.
     * @return A vector of ARION_MEM_EDIT instances representing the recorded memory accesses.
     */
    std::vector<std::shared_ptr<ARION_MEM_EDIT>> get_edits();
};

/// This class is responsible for managing memory mappings within an Arion emulation session. It allows the user to map
/// and unmap memory regions, change their protection flags, and read/write from/to arbitrary addresses.
class ARION_EXPORT MemoryManager
{
  private:
    /// The Arion instance associated with this instance.
    std::weak_ptr<Arion> arion;
    /// The size of a memory page in bytes. Used when allocating new pages.
    size_t page_sz;
    /// The current break address (brk). This is used to determine where new memory should be allocated.
    ADDR brk = 0;
    /// A vector of all mappings currently present within the Arion instance.
    std::vector<std::shared_ptr<ARION_MAPPING>> mappings;
    /**
     * Converts Arion memory protection rights to Unicorn ones.
     * @param[in] perms Arion memory protection rights.
     * @return Unicorn memory protection rights.
     */
    uint32_t to_uc_perms(PROT_FLAGS perms);
    /**
     * Inserts a new mapping in Arion memory.
     * @param[in] mapping The ARION_MAPPING to insert.
     */
    void insert_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    /**
     * Removes a mapping from Arion memory.
     * @param[in] mapping The ARION_MAPPING to remove.
     */
    void remove_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    /**
     * Merges contiguous memory mappings in a given range.
     * @param[in] start Start memory address of the mappings that should be merged.
     * @param[in] end End memory address of the mappings that should be merged.
     */
    void merge_uc_mappings(ADDR start, ADDR end);
    /**
     * Merges all contiguous memory mappings (expensive operation).
     */
    void merge_contiguous_uc_mappings();

  public:
    /// Used to record memory accesses to the associated Arion instance.
    std::unique_ptr<MemoryRecorder> recorder;
    /**
     * Builder for MemoryManager instances.
     * @param[in] arion The Arion instance associated with this instance.
     * @param[in] page_sz The size of a memory page in bytes. Used when allocating new pages.
     */
    MemoryManager(std::weak_ptr<Arion> arion, size_t page_sz) : arion(arion), page_sz(page_sz) {};
    /**
     * Instanciates and initializes new MemoryManager objects with some parameters.
     * @param[in] arion The Arion instance associated with this instance.
     * @return A new MemoryManager instance.
     */
    static std::unique_ptr<MemoryManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Aligns a given address to the next page boundary.
     * @param[in] addr Address to align.
     * @return Address aligned to the page size.
     */
    ADDR align_up(ADDR addr);
    /**
     * Checks whether a given address is mapped in Arion memory.
     * @param[in] addr Address to check.
     * @return True if the address is mapped, false otherwise.
     */
    bool ARION_EXPORT is_mapped(ADDR addr);
    /**
     * Checks whether a mapping exists in the current Arion memory layout.
     * @param[in] mapping The ARION_MAPPING to check.
     * @return True if the mapping exists, false otherwise.
     */
    bool ARION_EXPORT has_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    /**
     * Checks whether a memory region can be mapped at a given address and size.
     * @param[in] addr Desired starting address.
     * @param[in] sz Size of the region to map.
     * @return True if the mapping can be created, false otherwise.
     */
    bool ARION_EXPORT can_map(ADDR addr, size_t sz);
    /**
     * Checks if a mapping with a specific info string exists.
     * @param[in] info The info string associated with a mapping.
     * @return True if such a mapping exists, false otherwise.
     */
    bool ARION_EXPORT has_mapping_with_info(std::string info);
    /**
     * Retrieves a mapping by its info string.
     * @param[in] info The info string identifying the mapping.
     * @return The corresponding ARION_MAPPING if found, otherwise nullptr.
     */
    std::shared_ptr<ARION_MAPPING> ARION_EXPORT get_mapping_by_info(std::string info);
    /**
     * Retrieves the mapping that contains a given address.
     * @param[in] addr The address to look up.
     * @return The corresponding ARION_MAPPING if found, otherwise nullptr.
     */
    std::shared_ptr<ARION_MAPPING> ARION_EXPORT get_mapping_at(ADDR addr);
    /**
     * Returns a vector containing all currently active memory mappings.
     * @return A vector of shared pointers to ARION_MAPPING structures.
     */
    std::vector<std::shared_ptr<ARION_MAPPING>> ARION_EXPORT get_mappings();
    /**
     * Returns a string representation of all mappings.
     * @return A formatted string describing each memory mapping.
     */
    std::string ARION_EXPORT mappings_str();
    /**
     * Maps a memory region at a specific address.
     * @param[in] start_addr The starting address for the mapping.
     * @param[in] sz Size of the region to map.
     * @param[in] perms Memory protection flags.
     * @param[in] info Optional string identifying the mapping.
     * @return The starting address of the mapped region.
     */
    ADDR ARION_EXPORT map(ADDR start_addr, size_t sz, PROT_FLAGS perms, std::string info = "");
    /**
     * Maps a memory region near a specific address, automatically finding a suitable place.
     * @param[in] addr Preferred starting address.
     * @param[in] sz Size of the region to map.
     * @param[in] perms Memory protection flags.
     * @param[in] asc If true, search upwards in memory; otherwise downwards.
     * @param[in] info Optional info string for the mapping.
     * @return The address where the region was mapped.
     */
    ADDR ARION_EXPORT map_anywhere(ADDR addr, size_t sz, PROT_FLAGS perms, bool asc = true, std::string info = "");
    /**
     * Maps a memory region at any available location.
     * @param[in] sz Size of the region to map.
     * @param[in] perms Memory protection flags.
     * @param[in] asc If true, search upwards in memory; otherwise downwards.
     * @param[in] info Optional info string for the mapping.
     * @return The address where the region was mapped.
     */
    ADDR ARION_EXPORT map_anywhere(size_t sz, PROT_FLAGS perms, bool asc = true, std::string info = "");
    /**
     * Unmaps a memory range from an existing mapping.
     * @param[in] mapping The mapping to unmap from.
     * @param[in] start_addr Start of the range to unmap.
     * @param[in] end_addr End of the range to unmap.
     */
    void ARION_EXPORT unmap(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr);
    /**
     * Unmaps an entire mapping.
     * @param[in] mapping The mapping to unmap.
     */
    void ARION_EXPORT unmap(std::shared_ptr<ARION_MAPPING> mapping);
    /**
     * Unmaps a range of memory.
     * @param[in] start_addr Start address of the region to unmap.
     * @param[in] end_addr End address of the region to unmap.
     */
    void ARION_EXPORT unmap(ADDR start_addr, ADDR end_addr);
    /**
     * Unmaps the mapping containing a specific address.
     * @param[in] seg_addr Any address within the mapping to remove.
     */
    void ARION_EXPORT unmap(ADDR seg_addr);
    /**
     * Unmaps all memory regions.
     */
    void ARION_EXPORT unmap_all();
    /**
     * Changes protection flags for a specific region within a mapping.
     * @param[in] mapping The mapping to modify.
     * @param[in] start_addr Start of the region.
     * @param[in] end_addr End of the region.
     * @param[in] perms New protection flags.
     */
    void ARION_EXPORT protect(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr, PROT_FLAGS perms);
    /**
     * Changes protection flags for an entire mapping.
     * @param[in] mapping The mapping to modify.
     * @param[in] perms New protection flags.
     */
    void ARION_EXPORT protect(std::shared_ptr<ARION_MAPPING> mapping, PROT_FLAGS perms);
    /**
     * Changes protection flags for a memory range.
     * @param[in] start_addr Start of the range.
     * @param[in] end_addr End of the range.
     * @param[in] perms New protection flags.
     */
    void ARION_EXPORT protect(ADDR start_addr, ADDR end_addr, PROT_FLAGS perms);
    /**
     * Changes protection flags for the mapping containing a specific address.
     * @param[in] seg_addr Address within the mapping to modify.
     * @param[in] perms New protection flags.
     */
    void ARION_EXPORT protect(ADDR seg_addr, PROT_FLAGS perms);
    /**
     * Resizes an existing mapping to a new range.
     * @param[in] mapping The mapping to resize.
     * @param[in] start_addr New start address.
     * @param[in] end_addr New end address.
     */
    void ARION_EXPORT resize_mapping(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr);
    /**
     * Resizes the mapping containing a specific address.
     * @param[in] seg_addr Address within the mapping.
     * @param[in] start_addr New start address.
     * @param[in] end_addr New end address.
     */
    void ARION_EXPORT resize_mapping(ADDR seg_addr, ADDR start_addr, ADDR end_addr);
    /**
     * Reads raw bytes from memory.
     * @param[in] addr Starting address.
     * @param[in] data_sz Number of bytes to read.
     * @return Vector containing the read bytes.
     */
    std::vector<BYTE> ARION_EXPORT read(ADDR addr, size_t data_sz);
    /**
     * Reads an integer value of a given size from memory.
     * @param[in] addr Address to read from.
     * @param[in] n Number of bytes to read.
     * @return The read value as a 64-bit integer.
     */
    uint64_t ARION_EXPORT read_val(ADDR addr, uint8_t n);
    /**
     * Reads a pointer-sized value from memory.
     * @param[in] addr Address to read from.
     * @return Pointer value read from memory.
     */
    ADDR ARION_EXPORT read_ptr(ADDR addr);
    /**
     * Reads a size_t value from memory.
     * @param[in] addr Address to read from.
     * @return Size value read from memory.
     */
    size_t ARION_EXPORT read_sz(ADDR addr);
    /**
     * Reads a file descriptor (int) from memory.
     * @param[in] addr Address to read from.
     * @return File descriptor value.
     */
    int ARION_EXPORT read_fd(ADDR addr);
    /**
     * Reads memory as a hexadecimal string.
     * @param[in] addr Starting address.
     * @param[in] data_sz Number of bytes to read.
     * @param[in] sep Optional separator character between bytes.
     * @return String representation of the data in hexadecimal.
     */
    std::string ARION_EXPORT read_hex(ADDR addr, size_t data_sz, char sep = 0);
    /**
     * Reads a memory region and returns its ASCII representation.
     * @param[in] addr Starting address.
     * @param[in] data_sz Number of bytes to read.
     * @return ASCII string.
     */
    std::string ARION_EXPORT read_ascii(ADDR addr, size_t data_sz);
    /**
     * Reads a null-terminated C string from memory.
     * @param[in] addr Address of the string.
     * @return The read C string.
     */
    std::string ARION_EXPORT read_c_string(ADDR addr);
    /**
     * Reads an array of pointers from memory.
     * @param[in] addr Starting address of the array.
     * @return Vector of pointer values.
     */
    std::vector<ADDR> ARION_EXPORT read_ptr_arr(ADDR addr);
    /**
     * Disassembles a number of instructions from a given address.
     * @param[in] addr Starting address.
     * @param[in] count Number of instructions to read.
     * @return Vector of Capstone instruction objects.
     */
    std::vector<cs_insn> ARION_EXPORT read_instrs(ADDR addr, size_t count);
    /**
     * Writes raw bytes to memory.
     * @param[in] addr Destination address.
     * @param[in] data Pointer to the data.
     * @param[in] data_sz Size of the data.
     */
    void ARION_EXPORT write(ADDR addr, BYTE *data, size_t data_sz);
    /**
     * Writes a string to memory (without null-termination unless included).
     * @param[in] addr Destination address.
     * @param[in] data The string to write.
     */
    void ARION_EXPORT write_string(ADDR addr, std::string data);
    /**
     * Writes a fixed-size integer value to memory.
     * @param[in] addr Destination address.
     * @param[in] val The value to write.
     * @param[in] n Number of bytes to write.
     */
    void ARION_EXPORT write_val(ADDR addr, uint64_t val, uint8_t n);
    /**
     * Writes a pointer to memory.
     * @param[in] addr Destination address.
     * @param[in] ptr Pointer value to write.
     */
    void ARION_EXPORT write_ptr(ADDR addr, ADDR ptr);
    /**
     * Writes a size_t value to memory.
     * @param[in] addr Destination address.
     * @param[in] sz Size value to write.
     */
    void ARION_EXPORT write_sz(ADDR addr, size_t sz);
    /**
     * Writes a file descriptor (int) to memory.
     * @param[in] addr Destination address.
     * @param[in] fd File descriptor to write.
     */
    void ARION_EXPORT write_fd(ADDR addr, int fd);
    /**
     * Pushes a 64-bit value onto the emulated stack.
     * @param[in] val Value to push.
     */
    void ARION_EXPORT stack_push(uint64_t val);
    /**
     * Pushes raw bytes onto the emulated stack.
     * @param[in] data Pointer to the data to push.
     * @param[in] data_sz Size of the data in bytes.
     */
    void ARION_EXPORT stack_push_bytes(BYTE *data, size_t data_sz);
    /**
     * Pushes a string onto the emulated stack.
     * @param[in] data String to push.
     */
    void ARION_EXPORT stack_push_string(std::string data);
    /**
     * Aligns the emulated stack pointer to the current word or ABI boundary.
     */
    void ARION_EXPORT stack_align();
    /**
     * Pops a 64-bit value from the emulated stack.
     * @return The popped value.
     */
    uint64_t ARION_EXPORT stack_pop();
    /**
     * Sets the current program break (brk).
     * @param[in] brk New break address.
     */
    void ARION_EXPORT set_brk(ADDR brk);
    /**
     * Retrieves the current program break (brk).
     * @return The current break address.
     */
    ADDR ARION_EXPORT get_brk();
};

}; // namespace arion

#endif // ARION_MEMORY_MANAGER_HPP
