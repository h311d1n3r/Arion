#include <arion/common/code_tracer.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/components/code_trace_analysis.hpp>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>

using namespace arion;

void CodeTraceReader::read_header()
{
    char magic[sizeof(TRACER_FILE_MAGIC)];
    this->trace_f.read(magic, sizeof(TRACER_FILE_MAGIC) - 1);
    if (strncmp(magic, TRACER_FILE_MAGIC, sizeof(TRACER_FILE_MAGIC) - 1))
        throw WrongTraceFileMagicException(this->trace_path);
    this->trace_f.read((char *)&this->version, sizeof(float));
    if (this->version > TRACER_FILE_VERSION)
        throw NewerTraceFileVersionException(this->trace_path);
    this->trace_f.read((char *)&this->mode, sizeof(TRACE_MODE));
    if (this->mode >= TRACE_MODE::UNKNOWN)
        throw UnknownTraceModeException();
    this->trace_f.read((char *)&this->total_hits, sizeof(size_t));
    this->trace_f.read((char *)&this->secs_table_off, sizeof(off_t));
}

void CodeTraceReader::read_sections_table()
{
    this->trace_f.seekg(this->secs_table_off);
    this->trace_f.read((char *)&this->mod_sec_off, sizeof(off_t));
    this->trace_f.read((char *)&this->regs_sec_off, sizeof(off_t));
    this->trace_f.read((char *)&this->data_sec_off, sizeof(off_t));
}

void CodeTraceReader::read_modules_section()
{
    this->trace_f.seekg(this->mod_sec_off);
    uint16_t mappings_sz;
    this->trace_f.read((char *)&mappings_sz, sizeof(uint16_t));
    for (uint16_t mod_id = 0; mod_id < mappings_sz; mod_id++)
    {
        std::unique_ptr<TRACE_MODULE> module = std::make_unique<TRACE_MODULE>();
        module->mod_id = mod_id;
        uint16_t mapping_name_len;
        this->trace_f.read((char *)&mapping_name_len, sizeof(uint16_t));
        char *mapping_name = (char *)malloc(mapping_name_len);
        this->trace_f.read(mapping_name, mapping_name_len);
        module->name = std::string(mapping_name, mapping_name_len);
        free(mapping_name);
        this->trace_f.read((char *)&module->start, sizeof(ADDR));
        uint32_t mapping_len;
        this->trace_f.read((char *)&mapping_len, sizeof(mapping_len));
        module->end = module->start + mapping_len;
        uint16_t mod_hash_len;
        this->trace_f.read((char *)&mod_hash_len, sizeof(uint16_t));
        char *mod_hash = (char *)malloc(mod_hash_len);
        this->trace_f.read(mod_hash, mod_hash_len);
        module->hash = std::string(mod_hash, mod_hash_len);
        free(mod_hash);
        this->modules[mod_id] = std::move(module);
    }
}

void CodeTraceReader::read_regs_section()
{
    if (!this->regs_sec_off)
        return;

    this->trace_f.seekg(this->regs_sec_off);
    size_t ctxt_regs_sz;
    this->trace_f.read((char *)&ctxt_regs_sz, sizeof(size_t));
    REG reg;
    for (size_t reg_i = 0; reg_i < ctxt_regs_sz; reg_i++)
    {
        this->trace_f.read((char *)&reg, sizeof(REG));
        this->ctxt_regs.push_back(reg);
    }
}

void CodeTraceReader::prepare_file()
{
    this->read_header();
    this->read_sections_table();
    this->read_modules_section();
    this->read_regs_section();
    this->trace_f.seekg(this->data_sec_off);
}

CodeTraceReader::CodeTraceReader(std::string trace_path) : trace_path(trace_path)
{
    if (!std::filesystem::exists(trace_path))
        throw FileNotFoundException(trace_path);
    this->trace_f.open(trace_path, std::ios::binary | std::ios::in);
    if (!this->trace_f.is_open())
        throw FileOpenException(trace_path);
    this->trace_f_sz = std::filesystem::file_size(trace_path);
    if (this->trace_f_sz < TRACER_FILE_HEADER_SIZE)
        throw FileTooSmallException(this->trace_path, this->trace_f_sz, TRACER_FILE_HEADER_SIZE);
    this->prepare_file();
    this->hit_i = 0;
}

std::unique_ptr<CODE_HIT> CodeTraceReader::curr_hit()
{
    if (this->hit_i >= this->total_hits)
        return nullptr;

    if (this->hit_i >= 1)
    {
        this->trace_f.seekg((off_t)this->trace_f.tellg() -
                            (sizeof(uint32_t) + sizeof(uint16_t) * 2 + sizeof(RVAL) * this->ctxt_regs.size()));
        this->hit_i--;
    }
    return std::move(this->next_hit());
}

std::unique_ptr<CODE_HIT> CodeTraceReader::next_hit()
{
    if (this->hit_i >= this->total_hits)
        return nullptr;
    std::unique_ptr<CODE_HIT> hit = std::make_unique<CODE_HIT>();
    this->trace_f.read((char *)&hit->off, sizeof(uint32_t));
    this->trace_f.read((char *)&hit->sz, sizeof(uint16_t));
    this->trace_f.read((char *)&hit->mod_id, sizeof(uint16_t));
    hit->regs = std::make_unique<std::map<REG, RVAL>>();
    for (REG reg : this->ctxt_regs)
    {
        RVAL rval;
        this->trace_f.read((char *)&rval, sizeof(RVAL));
        hit->regs->operator[](reg) = rval;
    }
    this->hit_i++;
    return hit;
}

std::unique_ptr<CODE_HIT> CodeTraceReader::next_mod_hit(uint16_t mod_id)
{
    if (this->hit_i >= this->total_hits)
        return nullptr;

    std::unique_ptr<CODE_HIT> hit;
    while ((hit = this->next_hit()))
        if (hit->mod_id == mod_id)
            return hit;

    return nullptr;
}

std::unique_ptr<CODE_HIT> CodeTraceReader::reach_addr(arion::ADDR addr)
{
    std::unique_ptr<CODE_HIT> hit = this->curr_hit();
    if (!hit)
        return nullptr;

    do
    {
        std::unique_ptr<TRACE_MODULE> mod = this->get_module(hit->mod_id);
        if (mod->start + hit->off == addr)
            return std::move(hit);
    } while ((hit = this->next_hit()));
    return nullptr;
}

std::unique_ptr<CODE_HIT> CodeTraceReader::reach_off(uint16_t mod_id, uint32_t off)
{
    std::unique_ptr<CODE_HIT> hit = this->curr_hit();
    if (!hit)
        return nullptr;

    do
    {
        if (hit->off == off)
            return std::move(hit);
    } while ((hit = this->next_mod_hit(mod_id)));
    return nullptr;
}

off_t CodeTraceReader::get_hit_index()
{
    return this->hit_i - 1;
}

void CodeTraceReader::set_hit_index(off_t hit_i)
{
    this->hit_i = hit_i;
    this->trace_f.seekg(this->data_sec_off +
                        (sizeof(uint32_t) + sizeof(uint16_t) * 2 + sizeof(RVAL) * this->ctxt_regs.size()) * hit_i);
    this->next_hit();
}

void CodeTraceReader::reset_hit_cursor()
{
    this->hit_i = 0;
    this->trace_f.seekg(this->data_sec_off);
}

TRACE_MODE CodeTraceReader::get_mode()
{
    return this->mode;
}

std::unique_ptr<TRACE_MODULE> CodeTraceReader::get_module(uint16_t mod_id)
{
    auto mod_it = this->modules.find(mod_id);
    if (mod_it == this->modules.end())
        throw UnknownTraceModuleIdException(this->trace_path, mod_id);

    std::unique_ptr<TRACE_MODULE> mod = std::make_unique<TRACE_MODULE>(mod_it->second.get());
    return std::move(mod);
}

std::unique_ptr<TRACE_MODULE> CodeTraceReader::find_module_from_name(std::string name)
{
    for (auto &mod_it : this->modules)
        if (mod_it.second->name == name)
            return std::move(std::make_unique<TRACE_MODULE>(mod_it.second.get()));

    for (auto &mod_it : this->modules)
        if (mod_it.second->name.find(name) != std::string::npos)
            return std::move(std::make_unique<TRACE_MODULE>(mod_it.second.get()));

    throw UnknownTraceModuleNameException(this->trace_path, name);
}

std::unique_ptr<TRACE_MODULE> CodeTraceReader::find_module_from_hash(std::string hash)
{
    for (auto &mod_it : this->modules)
        if (mod_it.second->hash == hash)
            return std::move(std::make_unique<TRACE_MODULE>(mod_it.second.get()));

    throw UnknownTraceModuleHashException(this->trace_path, hash);
}

bool CodeTraceReader::has_reg(REG reg)
{
    return std::find(this->ctxt_regs.begin(), this->ctxt_regs.end(), reg) != this->ctxt_regs.end();
}

void CodeTraceAnalyzer::reach_address(arion::ADDR addr)
{
    if (!this->reader.reach_addr(addr))
        throw CantReachTraceAddrException(addr);
}

void CodeTraceAnalyzer::reach_offset(std::string name, uint32_t off)
{
    std::unique_ptr<TRACE_MODULE> mod = this->reader.find_module_from_name(name);

    if (!this->reader.reach_off(mod->mod_id, off))
        throw CantReachTraceOffException(mod->name, off);
}

void CodeTraceAnalyzer::loop_on_every_hit(ANALYZER_HIT_CALLBACK callback, bool reset_cursor)
{
    if (reset_cursor)
        this->reader.reset_hit_cursor();
    std::unique_ptr<CODE_HIT> hit;
    while ((hit = this->reader.next_hit()))
    {
        std::unique_ptr<TRACE_MODULE> mod = this->reader.get_module(hit->mod_id);
        if (!callback(std::make_unique<ANALYSIS_HIT>(this->reader.get_hit_index(), mod->name, hit->off, hit->sz,
                                                     hit->regs.get())))
            return;
    }
}

void CodeTraceAnalyzer::loop_on_every_mod_hit(ANALYZER_HIT_CALLBACK callback, std::string name, bool reset_cursor)
{
    if (reset_cursor)
        this->reader.reset_hit_cursor();
    std::unique_ptr<TRACE_MODULE> mod = this->reader.find_module_from_name(name);
    std::unique_ptr<CODE_HIT> hit;
    while ((hit = this->reader.next_mod_hit(mod->mod_id)))
    {
        if (!callback(std::make_unique<ANALYSIS_HIT>(this->reader.get_hit_index(), mod->name, hit->off, hit->sz,
                                                     hit->regs.get())))
            return;
    }
}

void CodeTraceAnalyzer::search_hit_address(ANALYZER_HIT_CALLBACK callback, ADDR addr, bool reset_cursor)
{
    if (reset_cursor)
        this->reader.reset_hit_cursor();
    std::unique_ptr<CODE_HIT> hit;
    while ((hit = this->reader.next_hit()))
    {
        std::unique_ptr<TRACE_MODULE> mod = this->reader.get_module(hit->mod_id);
        ADDR hit_addr_start = mod->start + hit->off;
        ADDR hit_addr_end = hit_addr_start + hit->sz;
        if (addr >= hit_addr_start && addr < hit_addr_end)
        {
            if (!callback(std::make_unique<ANALYSIS_HIT>(this->reader.get_hit_index(), mod->name, hit->off, hit->sz,
                                                         hit->regs.get())))
                return;
        }
    }
}

void CodeTraceAnalyzer::search_hit_address_range(ANALYZER_HIT_CALLBACK callback, ADDR start_addr, ADDR end_addr,
                                                 bool reset_cursor)
{
    if (reset_cursor)
        this->reader.reset_hit_cursor();
    std::unique_ptr<CODE_HIT> hit;
    while ((hit = this->reader.next_hit()))
    {
        std::unique_ptr<TRACE_MODULE> mod = this->reader.get_module(hit->mod_id);
        ADDR hit_addr_start = mod->start + hit->off;
        ADDR hit_addr_end = hit_addr_start + hit->sz;
        if ((hit_addr_start >= start_addr && hit_addr_start < end_addr) ||
            (hit_addr_end > start_addr && hit_addr_end <= end_addr) ||
            (start_addr >= hit_addr_start && start_addr < hit_addr_end))
        {
            if (!callback(std::make_unique<ANALYSIS_HIT>(this->reader.get_hit_index(), mod->name, hit->off, hit->sz,
                                                         hit->regs.get())))
                return;
        }
    }
}

void CodeTraceAnalyzer::search_hit_offset(ANALYZER_HIT_CALLBACK callback, std::string name, uint32_t off,
                                          bool reset_cursor)
{
    std::unique_ptr<TRACE_MODULE> mod = this->reader.find_module_from_name(name);
    ADDR addr = mod->start + off;
    this->search_hit_address(callback, addr, reset_cursor);
}

void CodeTraceAnalyzer::search_hit_offset_range(ANALYZER_HIT_CALLBACK callback, std::string name, ADDR start_off,
                                                ADDR end_off, bool reset_cursor)
{
    std::unique_ptr<TRACE_MODULE> mod = this->reader.find_module_from_name(name);
    ADDR start_addr = mod->start + start_off;
    ADDR end_addr = mod->start + end_off;
    this->search_hit_address_range(callback, start_addr, end_addr, reset_cursor);
}

CodeTraceComparator::CodeTraceComparator(std::string trace_path1, std::string trace_path2)
    : reader1(trace_path1), reader2(trace_path2)
{
    if (reader1.get_mode() != reader2.get_mode())
        throw DifferentTraceModesException();
}

void CodeTraceComparator::merge_at_address(arion::ADDR addr)
{
    if (!this->reader1.reach_addr(addr) || !this->reader2.reach_addr(addr))
        throw CantReachTraceAddrException(addr);
}

void CodeTraceComparator::merge_at_offset(std::string name, uint32_t off)
{
    std::unique_ptr<TRACE_MODULE> mod1 = this->reader1.find_module_from_name(name);
    std::unique_ptr<TRACE_MODULE> mod2 = this->reader2.find_module_from_hash(mod1->hash);

    if (!this->reader1.reach_off(mod1->mod_id, off) || !this->reader2.reach_off(mod2->mod_id, off))
        throw CantReachTraceOffException(mod1->name, off);
}

void CodeTraceComparator::search_uneq_hit_offset(COMPARATOR_HIT_CALLBACK callback, bool reset_cursors)
{
    if (reset_cursors)
    {
        this->reader1.reset_hit_cursor();
        this->reader2.reset_hit_cursor();
    }

    std::unique_ptr<CODE_HIT> hit1;
    std::unique_ptr<CODE_HIT> hit2;
    bool out_of_sync = false;
    while ((hit1 = this->reader1.next_hit()) && (hit2 = this->reader2.next_hit()))
    {
        std::unique_ptr<TRACE_MODULE> mod1 = this->reader1.get_module(hit1->mod_id);
        std::unique_ptr<TRACE_MODULE> mod2 = this->reader2.get_module(hit2->mod_id);
        if (hit1->off != hit2->off || mod1->hash != mod2->hash)
        {
            if (!out_of_sync && !callback(std::make_unique<ANALYSIS_HIT>(this->reader1.get_hit_index(), mod1->name,
                                                                         hit1->off, hit1->sz, hit1->regs.get()),
                                          std::make_unique<ANALYSIS_HIT>(this->reader2.get_hit_index(), mod2->name,
                                                                         hit2->off, hit2->sz, hit2->regs.get())))
                return;
            out_of_sync = false;
            off_t reader1_saved_i = this->reader1.get_hit_index();
            uint16_t mod1_search_id = this->reader1.find_module_from_hash(mod2->hash)->mod_id;
            std::unique_ptr<CODE_HIT> _hit1 = this->reader1.reach_off(mod1_search_id, hit2->off);
            off_t reader1_new_i = this->reader1.get_hit_index();
            off_t reader2_saved_i = this->reader2.get_hit_index();
            uint16_t mod2_search_id = this->reader1.find_module_from_hash(mod1->hash)->mod_id;
            std::unique_ptr<CODE_HIT> _hit2 = this->reader2.reach_off(mod2_search_id, hit1->off);
            off_t reader2_new_i = this->reader2.get_hit_index();
            if (_hit1)
            {
                if (_hit2)
                {
                    if (reader1_new_i - reader1_saved_i < reader2_new_i - reader2_saved_i)
                    {
                        reader1.set_hit_index(reader1_new_i);
                        reader2.set_hit_index(reader2_saved_i);
                        continue;
                    }
                    reader1.set_hit_index(reader1_saved_i);
                    reader2.set_hit_index(reader2_new_i);
                    continue;
                }
                reader1.set_hit_index(reader1_new_i);
                reader2.set_hit_index(reader2_saved_i);
                continue;
            }
            if (_hit2)
            {
                reader1.set_hit_index(reader1_saved_i);
                reader2.set_hit_index(reader2_new_i);
                continue;
            }
            reader1.set_hit_index(reader1_saved_i);
            reader2.set_hit_index(reader2_saved_i);
            out_of_sync = true;
        }
        else
            out_of_sync = false;
    }
}

void CodeTraceComparator::search_uneq_hit_offset_mod(COMPARATOR_HIT_CALLBACK callback, std::string name,
                                                     bool reset_cursors)
{
    if (reset_cursors)
    {
        this->reader1.reset_hit_cursor();
        this->reader2.reset_hit_cursor();
    }

    std::unique_ptr<TRACE_MODULE> mod1 = this->reader1.find_module_from_name(name);
    std::unique_ptr<TRACE_MODULE> mod2 = this->reader2.find_module_from_hash(mod1->hash);

    std::unique_ptr<CODE_HIT> hit1;
    std::unique_ptr<CODE_HIT> hit2;
    bool out_of_sync = false;
    while ((hit1 = this->reader1.next_mod_hit(mod1->mod_id)) && (hit2 = this->reader2.next_mod_hit(mod2->mod_id)))
    {
        if (hit1->off != hit2->off)
        {
            if (!out_of_sync && !callback(std::make_unique<ANALYSIS_HIT>(this->reader1.get_hit_index(), mod1->name,
                                                                         hit1->off, hit1->sz, hit1->regs.get()),
                                          std::make_unique<ANALYSIS_HIT>(this->reader2.get_hit_index(), mod2->name,
                                                                         hit2->off, hit2->sz, hit2->regs.get())))
                return;
            out_of_sync = false;
            off_t reader1_saved_i = this->reader1.get_hit_index();
            std::unique_ptr<CODE_HIT> _hit1 = this->reader1.reach_off(mod1->mod_id, hit2->off);
            off_t reader1_new_i = this->reader1.get_hit_index();
            off_t reader2_saved_i = this->reader2.get_hit_index();
            std::unique_ptr<CODE_HIT> _hit2 = this->reader2.reach_off(mod2->mod_id, hit1->off);
            off_t reader2_new_i = this->reader2.get_hit_index();
            if (_hit1)
            {
                if (_hit2)
                {
                    if (reader1_new_i - reader1_saved_i < reader2_new_i - reader2_saved_i)
                    {
                        reader1.set_hit_index(reader1_new_i);
                        reader2.set_hit_index(reader2_saved_i);
                        continue;
                    }
                    reader1.set_hit_index(reader1_saved_i);
                    reader2.set_hit_index(reader2_new_i);
                    continue;
                }
                reader1.set_hit_index(reader1_new_i);
                reader2.set_hit_index(reader2_saved_i);
                continue;
            }
            if (_hit2)
            {
                reader1.set_hit_index(reader1_saved_i);
                reader2.set_hit_index(reader2_new_i);
                continue;
            }
            reader1.set_hit_index(reader1_saved_i);
            reader2.set_hit_index(reader2_saved_i);
            out_of_sync = true;
        }
        else
            out_of_sync = false;
    }
}
