#include <arion/arion.hpp>
#include <arion/common/code_tracer.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/utils/convert_utils.hpp>
#include <arion/utils/fs_utils.hpp>
#include <cstdint>
#include <filesystem>
#include <ios>
#include <iosfwd>
#include <memory>

using namespace arion;

static size_t anon_code_counter = 0;

std::unique_ptr<CodeTracer> CodeTracer::initialize(std::weak_ptr<Arion> arion)
{
    return std::move(std::make_unique<CodeTracer>(arion));
}

void CodeTracer::instr_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    arion->tracer->process_hit(addr, sz);
}

void CodeTracer::block_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data)
{
    arion->tracer->process_hit(addr, sz);
}

void CodeTracer::prepare_file()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (this->out_f.is_open())
        this->out_f.close();

    this->out_f.open(out_f_path, std::ios::binary | std::ios::out);
    if (!this->out_f.is_open())
        throw FileOpenException(out_f_path);

    switch (this->mode)
    {
    case TRACE_MODE::INSTR:
    case TRACE_MODE::CTXT:
    case TRACE_MODE::BLOCK: {
        // Start of Header
        this->out_f.write(TRACER_FILE_MAGIC, strlen(TRACER_FILE_MAGIC));
        this->out_f.write((char *)&TRACER_FILE_VERSION, sizeof(float));
        this->out_f.write((char *)&this->mode, sizeof(TRACE_MODE));
        this->total_hits_off = this->out_f.tellp();
        this->out_f.seekp(sizeof(size_t), std::ios::cur); // Reserved for later replacement of Total Hits
        uint8_t padding = 8 - (this->out_f.tellp() % 8);
        if (padding == 8)
            padding = 0;
        off_t secs_table_off = (off_t)this->out_f.tellp() + padding + sizeof(off_t);
        this->out_f.write((char *)&secs_table_off, sizeof(off_t));
        this->out_f.write((char *)&EMPTY, padding);

        // Start of Sections Table
        this->mod_sec_off = this->out_f.tellp();
        this->out_f.seekp(sizeof(off_t), std::ios::cur); // Reserved for later replacement of Modules Section
        off_t regs_sec_off = 0;
        if (this->mode == TRACE_MODE::CTXT)
            regs_sec_off = (off_t)this->out_f.tellp() + sizeof(off_t) * 2;
        this->out_f.write((char *)&regs_sec_off, sizeof(off_t));
        std::vector<REG> ctxt_regs = arion->arch->get_context_regs();
        size_t ctxt_regs_sz = ctxt_regs.size();
        off_t data_sec_off = (off_t)this->out_f.tellp() + sizeof(off_t) + sizeof(size_t) + ctxt_regs_sz * sizeof(REG);
        this->out_f.write((char *)&data_sec_off, sizeof(off_t));

        // Start of Registers Section
        this->out_f.write((char *)&ctxt_regs_sz, sizeof(size_t));
        for (REG reg : ctxt_regs)
            this->out_f.write((char *)&reg, sizeof(REG));
        break;
    }
    case TRACE_MODE::DRCOV:
        break;
    case TRACE_MODE::UNKNOWN:
    default:
        throw UnknownTraceModeException();
        break;
    }
}

void CodeTracer::release_file()
{
    switch (this->mode)
    {
    case TRACE_MODE::INSTR:
    case TRACE_MODE::CTXT:
    case TRACE_MODE::BLOCK: {
        off_t curr_pos = this->out_f.tellp();
        this->out_f.seekp(this->total_hits_off);
        this->out_f.write((char *)&this->total_hits, sizeof(size_t));
        this->out_f.seekp(this->mod_sec_off);
        this->out_f.write((char *)&curr_pos, sizeof(off_t));
        this->out_f.seekp(curr_pos);
        uint16_t mappings_sz = this->mappings.size();
        this->out_f.write((char *)&mappings_sz, sizeof(uint16_t));
        for (std::unique_ptr<TRACER_MAPPING> &mapping : this->mappings)
        {
            uint16_t mapping_name_len = mapping->name.length();
            this->out_f.write((char *)&mapping_name_len, sizeof(uint16_t));
            this->out_f.write(mapping->name.c_str(), mapping_name_len);
            this->out_f.write((char *)&mapping->start, sizeof(ADDR));
            uint32_t mapping_len = mapping->end - mapping->start;
            this->out_f.write((char *)&mapping_len, sizeof(uint32_t));
            std::string mod_hash = md5_hash_file(mapping->name);
            uint16_t mod_hash_len = mod_hash.size();
            this->out_f.write((char *)&mod_hash_len, sizeof(uint16_t));
            this->out_f.write(mod_hash.c_str(), mod_hash_len);
        }
        break;
    }
    case TRACE_MODE::DRCOV: {
        this->out_f.close();
        std::string tmp_f_path = gen_tmp_path();
        std::ofstream tmp_f(tmp_f_path, std::ios::binary);
        tmp_f << "DRCOV VERSION: 2" << std::endl;
        tmp_f << "DRCOV FLAVOR: drcov" << std::endl;
        tmp_f << "Module Table: version 2, count " << std::dec << +this->mappings.size() << std::endl;
        tmp_f << "Columns: id, base, end, entry, checksum, timestamp, path" << std::endl;
        off_t mod_id = 0;
        for (std::unique_ptr<TRACER_MAPPING> &mapping : this->mappings)
        {
            tmp_f << " " << std::dec << +mod_id << ", " << int_to_hex<ADDR>(mapping->start) << ", "
                  << int_to_hex<ADDR>(mapping->end) << ", 0x0, 0x0, 0x0, " << mapping->name << std::endl;
            mod_id++;
        }
        tmp_f << "BB Table: " << std::dec << +this->total_hits << " bbs" << std::endl;
        read_bin_file(
            this->out_f_path, 0, std::filesystem::file_size(this->out_f_path),
            [&tmp_f](std::array<BYTE, ARION_BUF_SZ> buf, ADDR off, size_t sz) { tmp_f.write((char *)buf.data(), sz); });
        tmp_f.close();
        std::filesystem::copy(tmp_f_path, this->out_f_path, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(tmp_f_path);
        break;
    }
    case TRACE_MODE::UNKNOWN:
    default:
        throw UnknownTraceModeException();
        break;
    }

    if (this->out_f.is_open())
        this->out_f.close();
}

void CodeTracer::process_hit(ADDR addr, size_t sz)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    off_t mod_id = 0;
    ADDR mapping_start = 0;
    bool found_mapping = false;
    for (std::unique_ptr<TRACER_MAPPING> &mapping : this->mappings)
    {
        if (addr >= mapping->start && addr < mapping->end)
        {
            mapping_start = mapping->start;
            found_mapping = true;
            break;
        }
        mod_id++;
    }
    if (!found_mapping)
        return;

    this->total_hits++;
    std::unique_ptr<CODE_HIT> hit = std::make_unique<CODE_HIT>(addr - mapping_start, sz, mod_id);
    if (this->mode == TRACE_MODE::CTXT)
        hit->regs = std::move(arion->arch->dump_regs());
    this->hits.push_back(std::move(hit));

    size_t hits_sz = this->hits.size();
    size_t max_hits = this->mode == TRACE_MODE::CTXT ? MAX_HEAVY_HITS : MAX_LIGHT_HITS;
    if (hits_sz >= max_hits)
        this->flush_hits();
}

void CodeTracer::flush_hits()
{
    for (std::unique_ptr<CODE_HIT> &hit : this->hits)
    {
        this->out_f.write((char *)&hit->off, sizeof(uint32_t));
        this->out_f.write((char *)&hit->sz, sizeof(uint16_t));
        this->out_f.write((char *)&hit->mod_id, sizeof(uint16_t));
        if (this->mode == TRACE_MODE::CTXT)
            for (auto &reg : *hit->regs)
                this->out_f.write((char *)&reg.second, sizeof(RVAL));
    }
    this->hits.clear();
}

CodeTracer::~CodeTracer()
{
    if (this->enabled)
        this->stop();
}

void CodeTracer::start(std::string out_f_path, TRACE_MODE mode)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    anon_code_counter = 0;
    if (this->enabled)
        throw TracerAlreadyEnabledException();
    this->enabled = true;
    this->out_f_path = out_f_path;

    this->mode = mode;
    this->prepare_file();
    this->total_hits = 0;
    this->hits.clear();
    switch (mode)
    {
    case TRACE_MODE::INSTR:
    case TRACE_MODE::CTXT:
        this->curr_hook_id = arion->hooks->hook_code(instr_hook);
        break;
    case TRACE_MODE::BLOCK:
    case TRACE_MODE::DRCOV:
        this->curr_hook_id = arion->hooks->hook_block(instr_hook);
        break;
    case TRACE_MODE::UNKNOWN:
    default:
        throw UnknownTraceModeException();
    }
}

void CodeTracer::stop()
{
    if (!this->enabled)
        throw TracerAlreadyDisabledException();
    this->flush_hits();
    this->enabled = false;

    this->release_file();
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (arion) // Otherwise all hooks are cleared anyway
        arion->hooks->unhook(this->curr_hook_id);
    this->total_hits = 0;
}

void CodeTracer::process_new_mapping(std::shared_ptr<ARION_MAPPING> mapping)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (arion->is_running() && !this->enabled)
        return;

    // patch : custom user memory mapped segment can be without info but executable : must trace it
    if (mapping->info.empty() && mapping->perms & LINUX_EXEC_PERMS) {
        anon_code_counter++;
        mapping->info = "[code" + std::to_string(anon_code_counter) + "]";
    }

    if (!mapping->info.size())
        return;

    for (std::unique_ptr<TRACER_MAPPING> &tr_mapping : this->mappings)
    {
        if (mapping->info == tr_mapping->name)
        {
            if (mapping->start_addr < tr_mapping->start)
                tr_mapping->start = mapping->start_addr;
            if (mapping->end_addr > tr_mapping->end)
                tr_mapping->end = mapping->end_addr;
            return;
        }
    }

    this->mappings.push_back(std::make_unique<TRACER_MAPPING>(mapping->start_addr, mapping->end_addr, mapping->info));
}

bool CodeTracer::is_enabled()
{
    return this->enabled;
}

TRACE_MODE CodeTracer::get_mode()
{
    return this->mode;
}
