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

struct ARION_EXPORT ARION_MAPPING
{
    ADDR start_addr;
    ADDR end_addr;
    PROT_FLAGS perms;
    std::string info;
    BYTE *saved_data = nullptr;
    ARION_MAPPING() {};
    ARION_MAPPING(ADDR start_addr, ADDR end_addr, PROT_FLAGS perms, std::string info = "")
        : start_addr(start_addr), end_addr(end_addr), perms(perms), info(info) {};
    ARION_MAPPING(ARION_MAPPING *arion_m)
        : start_addr(arion_m->start_addr), end_addr(arion_m->end_addr), perms(arion_m->perms), info(arion_m->info),
          saved_data(arion_m->saved_data) {};
    ~ARION_MAPPING()
    {
        if (saved_data)
            free(saved_data);
        saved_data = nullptr;
    }
};
std::vector<BYTE> serialize_arion_mapping(ARION_MAPPING *arion_m);
ARION_MAPPING *deserialize_arion_mapping(std::vector<BYTE> srz_mapping);

struct ARION_MEM_EDIT
{
    ADDR addr;
    size_t sz;

    ARION_MEM_EDIT(ADDR addr, size_t sz) : addr(addr), sz(sz) {};
};

class MemoryRecorder
{
  private:
    std::weak_ptr<Arion> arion;
    std::vector<std::shared_ptr<ARION_MEM_EDIT>> edits;
    HOOK_ID write_mem_hook_id;
    bool started = false;
    static bool write_mem_hook(std::shared_ptr<Arion> arion, uc_mem_type type, uint64_t addr, int size, int64_t val,
                               void *user_data);
    void add_edit(std::shared_ptr<ARION_MEM_EDIT> edit);

  public:
    MemoryRecorder(std::weak_ptr<Arion> arion) : arion(arion), started(false) {};
    static std::unique_ptr<MemoryRecorder> initialize(std::weak_ptr<Arion> arion);
    void clear();
    void start();
    void stop();
    std::vector<std::shared_ptr<ARION_MEM_EDIT>> get_edits();
};

class ARION_EXPORT MemoryManager
{
  private:
    std::weak_ptr<Arion> arion;
    size_t page_sz;
    ADDR brk = 0;
    std::vector<std::shared_ptr<ARION_MAPPING>> mappings;
    uint32_t to_uc_perms(PROT_FLAGS perms);
    void insert_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    void remove_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    void merge_uc_mappings(ADDR start, ADDR end);
    void merge_contiguous_uc_mappings();

  public:
    std::unique_ptr<MemoryRecorder> recorder;
    MemoryManager(std::weak_ptr<Arion> arion, size_t page_sz) : arion(arion), page_sz(page_sz) {};
    static std::unique_ptr<MemoryManager> initialize(std::weak_ptr<Arion> arion);
    ADDR align_up(ADDR addr);
    bool ARION_EXPORT is_mapped(ADDR addr);
    bool ARION_EXPORT has_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    bool ARION_EXPORT can_map(ADDR addr, size_t sz);
    bool ARION_EXPORT has_mapping_with_info(std::string info);
    std::shared_ptr<ARION_MAPPING> ARION_EXPORT get_mapping_by_info(std::string info);
    std::shared_ptr<ARION_MAPPING> ARION_EXPORT get_mapping_at(ADDR addr);
    std::vector<std::shared_ptr<ARION_MAPPING>> ARION_EXPORT get_mappings();
    std::string ARION_EXPORT mappings_str();
    ADDR ARION_EXPORT map(ADDR start_addr, size_t sz, PROT_FLAGS perms, std::string info = "");
    ADDR ARION_EXPORT map_anywhere(ADDR addr, size_t sz, PROT_FLAGS perms, bool asc = true, std::string info = "");
    ADDR ARION_EXPORT map_anywhere(size_t sz, PROT_FLAGS perms, bool asc = true, std::string info = "");
    void ARION_EXPORT unmap(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr);
    void ARION_EXPORT unmap(std::shared_ptr<ARION_MAPPING> mapping);
    void ARION_EXPORT unmap(ADDR start_addr, ADDR end_addr);
    void ARION_EXPORT unmap(ADDR seg_addr);
    void ARION_EXPORT unmap_all();
    void ARION_EXPORT protect(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr, PROT_FLAGS perms);
    void ARION_EXPORT protect(std::shared_ptr<ARION_MAPPING> mapping, PROT_FLAGS perms);
    void ARION_EXPORT protect(ADDR start_addr, ADDR end_addr, PROT_FLAGS perms);
    void ARION_EXPORT protect(ADDR seg_addr, PROT_FLAGS perms);
    void ARION_EXPORT resize_mapping(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr);
    void ARION_EXPORT resize_mapping(ADDR seg_addr, ADDR start_addr, ADDR end_addr);
    std::vector<BYTE> ARION_EXPORT read(ADDR addr, size_t data_sz);
    uint64_t ARION_EXPORT read_val(ADDR addr, uint8_t n);
    ADDR ARION_EXPORT read_ptr(ADDR addr);
    size_t ARION_EXPORT read_sz(ADDR addr);
    int ARION_EXPORT read_fd(ADDR addr);
    std::string ARION_EXPORT read_hex(ADDR addr, size_t data_sz, char sep = 0);
    std::string ARION_EXPORT read_ascii(ADDR addr, size_t data_sz);
    std::string ARION_EXPORT read_c_string(ADDR addr);
    std::vector<ADDR> ARION_EXPORT read_ptr_arr(ADDR addr);
    std::vector<cs_insn> ARION_EXPORT read_instrs(ADDR addr, size_t count);
    void ARION_EXPORT write(ADDR addr, BYTE *data, size_t data_sz);
    void ARION_EXPORT write_string(ADDR addr, std::string data);
    void ARION_EXPORT write_val(ADDR addr, uint64_t val, uint8_t n);
    void ARION_EXPORT write_ptr(ADDR addr, ADDR ptr);
    void ARION_EXPORT write_sz(ADDR addr, size_t sz);
    void ARION_EXPORT write_fd(ADDR addr, int fd);
    void ARION_EXPORT stack_push(uint64_t val);
    void ARION_EXPORT stack_push_bytes(BYTE *data, size_t data_sz);
    void ARION_EXPORT stack_push_string(std::string data);
    void ARION_EXPORT stack_align();
    uint64_t ARION_EXPORT stack_pop();
    void ARION_EXPORT set_brk(ADDR brk);
    ADDR ARION_EXPORT get_brk();
};

}; // namespace arion

#endif // ARION_MEMORY_MANAGER_HPP
