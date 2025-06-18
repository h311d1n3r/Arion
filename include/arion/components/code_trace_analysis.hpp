#ifndef ARION_CODE_TRACE_ANALYSIS_HPP
#define ARION_CODE_TRACE_ANALYSIS_HPP

#include <arion/common/code_tracer.hpp>
#include <arion/common/global_defs.hpp>
#include <fstream>
#include <map>
#include <string>

struct TRACE_MODULE
{
    uint16_t mod_id;
    std::string name;
    std::string hash;
    arion::ADDR start;
    arion::ADDR end;

    TRACE_MODULE() {};
    TRACE_MODULE(uint16_t mod_id, std::string name, std::string hash, arion::ADDR start, arion::ADDR end)
        : mod_id(mod_id), name(name), hash(hash), start(start), end(end) {};
    TRACE_MODULE(TRACE_MODULE *mod)
        : mod_id(mod->mod_id), name(mod->name), hash(mod->hash), start(mod->start), end(mod->end) {};
};

class CodeTraceReader
{
  private:
    std::string trace_path;
    std::ifstream trace_f;
    size_t trace_f_sz;
    float version;
    TRACE_MODE mode;
    size_t total_hits;
    std::vector<arion::REG> ctxt_regs;
    off_t secs_table_off;
    off_t mod_sec_off;
    off_t regs_sec_off;
    off_t data_sec_off;
    off_t hit_i;
    std::map<uint16_t, std::unique_ptr<TRACE_MODULE>> modules;
    void read_header();
    void read_sections_table();
    void read_modules_section();
    void read_regs_section();
    void prepare_file();

  public:
    CodeTraceReader(std::string trace_path);
    std::unique_ptr<CODE_HIT> curr_hit();
    std::unique_ptr<CODE_HIT> next_hit();
    std::unique_ptr<CODE_HIT> next_mod_hit(uint16_t mod_id);
    std::unique_ptr<CODE_HIT> reach_addr(arion::ADDR addr);
    std::unique_ptr<CODE_HIT> reach_off(uint16_t mod_id, uint32_t off);
    off_t get_hit_index();
    void set_hit_index(off_t hit_i);
    void reset_hit_cursor();
    TRACE_MODE get_mode();
    std::unique_ptr<TRACE_MODULE> get_module(uint16_t mod_id);
    std::unique_ptr<TRACE_MODULE> find_module_from_name(std::string name);
    std::unique_ptr<TRACE_MODULE> find_module_from_hash(std::string hash);
    bool has_reg(arion::REG reg);
};

struct ARION_EXPORT ANALYSIS_HIT
{
    off_t hit_i;
    std::string mod_name;
    uint32_t off;
    uint16_t sz;
    std::unique_ptr<std::map<arion::REG, arion::RVAL>> regs;

    ANALYSIS_HIT();
    ANALYSIS_HIT(off_t hit_i, std::string mod_name, uint32_t off, uint16_t sz, std::map<arion::REG, arion::RVAL> *regs)
        : hit_i(hit_i), mod_name(mod_name), off(off), sz(sz),
          regs(std::make_unique<std::map<arion::REG, arion::RVAL>>(*regs)) {};
};

using ANALYZER_HIT_CALLBACK = std::function<bool(std::unique_ptr<ANALYSIS_HIT> hit)>;

class ARION_EXPORT CodeTraceAnalyzer
{
  private:
    CodeTraceReader reader;

  public:
    ARION_EXPORT CodeTraceAnalyzer(std::string trace_path);
    void ARION_EXPORT reach_address(arion::ADDR addr);
    void ARION_EXPORT reach_offset(std::string name, uint32_t off);
    void ARION_EXPORT loop_on_every_hit(ANALYZER_HIT_CALLBACK callback, bool reset_cursor = true);
    void ARION_EXPORT loop_on_every_mod_hit(ANALYZER_HIT_CALLBACK callback, std::string name, bool reset_cursor = true);
    void ARION_EXPORT search_hit_address(ANALYZER_HIT_CALLBACK callback, arion::ADDR addr, bool reset_cursor = true);
    void ARION_EXPORT search_hit_address_range(ANALYZER_HIT_CALLBACK callback, arion::ADDR start_addr,
                                               arion::ADDR end_addr, bool reset_cursor = true);
    void ARION_EXPORT search_hit_offset(ANALYZER_HIT_CALLBACK callback, std::string name, uint32_t off,
                                        bool reset_cursor = true);
    void ARION_EXPORT search_hit_offset_range(ANALYZER_HIT_CALLBACK callback, std::string name, arion::ADDR start_off,
                                              arion::ADDR end_off, bool reset_cursor = true);

    template <typename T> void ARION_EXPORT search_reg_val(ANALYZER_HIT_CALLBACK callback, arion::REG reg, T val)
    {
        if (this->reader.get_mode() != TRACE_MODE::CTXT)
            throw WrongTraceModeException();
        if (!this->reader.has_reg(reg))
            throw UnknownTraceRegException(reg);

        this->reader.reset_hit_cursor();
        std::unique_ptr<CODE_HIT> hit;
        while ((hit = this->reader.next_hit()))
        {
            arion::RVAL hit_val = hit->regs->at(reg);
            bool is_equal = false;
            if constexpr (std::is_same_v<T, arion::RVAL8>)
                if (hit_val.r8 != val)
                    continue;
            if constexpr (std::is_same_v<T, arion::RVAL16>)
                if (hit_val.r16 != val)
                    continue;
            if constexpr (std::is_same_v<T, arion::RVAL32>)
                if (hit_val.r32 != val)
                    continue;
            if constexpr (std::is_same_v<T, arion::RVAL64>)
                if (hit_val.r64 != val)
                    continue;
            if constexpr (std::is_same_v<T, arion::RVAL128>)
                if (hit_val.r128 != val)
                    continue;
            if constexpr (std::is_same_v<T, arion::RVAL256>)
                if (hit_val.r256 != val)
                    continue;
            if constexpr (std::is_same_v<T, arion::RVAL512>)
                if (hit_val.r512 != val)
                    continue;
            std::unique_ptr<TRACE_MODULE> mod = this->reader.get_module(hit->mod_id);
            if (!callback(std::make_unique<ANALYSIS_HIT>(mod->name, hit->off, hit->sz, hit->regs.get())))
                return;
        }
    }
};

using COMPARATOR_HIT_CALLBACK =
    std::function<bool(std::unique_ptr<ANALYSIS_HIT> hit1, std::unique_ptr<ANALYSIS_HIT> hit2)>;

class ARION_EXPORT CodeTraceComparator
{
  private:
    CodeTraceReader reader1, reader2;

  public:
    ARION_EXPORT CodeTraceComparator(std::string trace_path1, std::string trace_path2);
    void ARION_EXPORT merge_at_address(arion::ADDR addr);
    void ARION_EXPORT merge_at_offset(std::string name, uint32_t off);
    void ARION_EXPORT search_uneq_hit_offset(COMPARATOR_HIT_CALLBACK callback, bool reset_cursors = true);
    void ARION_EXPORT search_uneq_hit_offset_mod(COMPARATOR_HIT_CALLBACK callback, std::string name,
                                                 bool reset_cursors = true);
    void ARION_EXPORT search_uneq_reg(COMPARATOR_HIT_CALLBACK callback, bool reset_cursors = true);
};

#endif // ARION_CODE_TRACE_ANALYSIS_HPP
