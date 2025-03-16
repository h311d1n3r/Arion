#ifndef ARION_CODE_TRACER_HPP
#define ARION_CODE_TRACER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/common/memory_manager.hpp>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <string>

#define MAX_LIGHT_HITS 1024
#define MAX_HEAVY_HITS 64

class Arion;

const char TRACER_FILE_MAGIC[] = "ARIONTRC";
const float TRACER_FILE_VERSION = 1.0;
const uint16_t TRACER_FILE_HEADER_SIZE = 0x28;

enum ARION_EXPORT TRACE_MODE : uint8_t
{
    // Arion specific files
    INSTR,
    CTXT,
    BLOCK,

    // Non-Arion files
    DRCOV,

    // Used to mark end of enum
    UNKNOWN
};

struct ARION_EXPORT CODE_HIT
{
    uint32_t off;
    uint16_t sz;
    uint16_t mod_id;
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> regs;

    CODE_HIT() {};
    CODE_HIT(uint32_t off, size_t sz, uint16_t mod_id) : off(off), sz(sz), mod_id(mod_id) {};
};

struct TRACER_MAPPING
{
    arion::ADDR start;
    arion::ADDR end;
    std::string name;

    TRACER_MAPPING(arion::ADDR start, arion::ADDR end, std::string name) : start(start), end(end), name(name) {};
};

class ARION_EXPORT CodeTracer
{
  private:
    std::weak_ptr<Arion> arion;
    bool enabled = false;
    TRACE_MODE mode;
    HOOK_ID curr_hook_id;
    std::string out_f_path;
    std::ofstream out_f;
    size_t total_hits;
    off_t total_hits_off;
    off_t mod_sec_off;
    std::vector<std::unique_ptr<CODE_HIT>> hits;
    std::vector<std::unique_ptr<TRACER_MAPPING>> mappings;
    static void instr_hook(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data);
    static void block_hook(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data);
    void prepare_file();
    void release_file();
    void process_hit(arion::ADDR addr, size_t sz);
    void flush_hits();

  public:
    static std::unique_ptr<CodeTracer> initialize(std::weak_ptr<Arion> arion);
    CodeTracer(std::weak_ptr<Arion> arion) : arion(arion) {};
    ~CodeTracer();
    void ARION_EXPORT start(std::string out_f_path, TRACE_MODE mode);
    void ARION_EXPORT stop();
    void process_new_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    bool ARION_EXPORT is_enabled();
    TRACE_MODE ARION_EXPORT get_mode();
};

#endif // ARION_CODE_TRACER_HPP
