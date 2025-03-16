#include <algorithm>
#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/utils/convert_utils.hpp>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <memory>
#include <unicorn/unicorn.h>
#include <unistd.h>

using namespace arion;

std::vector<BYTE> serialize_arion_mapping(ARION_MAPPING *arion_m)
{
    std::vector<BYTE> srz_mapping;

    srz_mapping.insert(srz_mapping.end(), (BYTE *)&arion_m->start_addr, (BYTE *)&arion_m->start_addr + sizeof(ADDR));
    srz_mapping.insert(srz_mapping.end(), (BYTE *)&arion_m->end_addr, (BYTE *)&arion_m->end_addr + sizeof(ADDR));
    srz_mapping.insert(srz_mapping.end(), (BYTE *)&arion_m->perms, (BYTE *)&arion_m->perms + sizeof(PROT_FLAGS));
    size_t info_sz = arion_m->info.size();
    srz_mapping.insert(srz_mapping.end(), (BYTE *)&info_sz, (BYTE *)&info_sz + sizeof(size_t));
    srz_mapping.insert(srz_mapping.end(), (BYTE *)arion_m->info.c_str(), (BYTE *)arion_m->info.c_str() + info_sz);
    size_t mapping_sz = arion_m->end_addr - arion_m->start_addr;
    srz_mapping.insert(srz_mapping.end(), (BYTE *)arion_m->saved_data, (BYTE *)arion_m->saved_data + mapping_sz);

    return srz_mapping;
}

ARION_MAPPING *deserialize_arion_mapping(std::vector<BYTE> srz_mapping)
{
    ARION_MAPPING *arion_m = new ARION_MAPPING;

    off_t off = 0;
    memcpy(&arion_m->start_addr, srz_mapping.data() + off, sizeof(ADDR));
    off += sizeof(ADDR);
    memcpy(&arion_m->end_addr, srz_mapping.data() + off, sizeof(ADDR));
    off += sizeof(ADDR);
    memcpy(&arion_m->perms, srz_mapping.data() + off, sizeof(PROT_FLAGS));
    off += sizeof(PROT_FLAGS);
    size_t info_sz;
    memcpy(&info_sz, srz_mapping.data() + off, sizeof(size_t));
    off += sizeof(size_t);
    char *info = (char *)malloc(info_sz);
    memcpy(info, srz_mapping.data() + off, info_sz);
    arion_m->info = std::string(info, info_sz);
    free(info);
    off += info_sz;
    size_t mapping_sz = arion_m->end_addr - arion_m->start_addr;
    arion_m->saved_data = (BYTE *)malloc(mapping_sz);
    memcpy(arion_m->saved_data, srz_mapping.data() + off, mapping_sz);

    return arion_m;
}

std::unique_ptr<MemoryManager> MemoryManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::move(std::make_unique<MemoryManager>(arion, ARION_SYSTEM_PAGE_SZ));
}

bool MemoryManager::is_mapped(ADDR addr)
{
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
    {
        if (addr >= mapping->start_addr && addr < mapping->end_addr)
            return true;
    }
    return false;
}

bool MemoryManager::can_map(ADDR start_addr, size_t sz)
{
    ADDR end_addr = start_addr + sz;
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
    {
        if ((start_addr >= mapping->start_addr && start_addr < mapping->end_addr) ||
            (end_addr > mapping->start_addr && end_addr <= mapping->end_addr))
            return false;
    }
    return true;
}

bool MemoryManager::has_mapping_with_info(std::string info)
{
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
    {
        if (mapping->info == info)
            return true;
    }
    return false;
}

std::shared_ptr<ARION_MAPPING> MemoryManager::get_mapping_by_info(std::string info)
{
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
    {
        if (mapping->info == info)
            return mapping;
    }
    throw NoSegmentWithInfoException(info);
}

std::shared_ptr<ARION_MAPPING> MemoryManager::get_mapping_at(ADDR addr)
{
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
    {
        if (addr >= mapping->start_addr && addr < mapping->end_addr)
            return mapping;
    }
    throw NoSegmentAtAddrException(addr);
}

std::vector<std::shared_ptr<ARION_MAPPING>> MemoryManager::get_mappings()
{
    return this->mappings;
}

std::string MemoryManager::mappings_str()
{
    std::stringstream ss;
    ss << std::left << std::setw(19) << "[START]";
    ss << std::setw(19) << "[END]";
    ss << std::setw(8) << "[FLAGS]";
    ss << "[INFO]" << std::endl;

    size_t mappings_sz = this->mappings.size();
    for (size_t m_i = 0; m_i < mappings_sz; m_i++)
    {
        std::shared_ptr<ARION_MAPPING> mapping = this->mappings.at(m_i);
        ss << std::left << std::setw(19) << int_to_hex(mapping->start_addr, 16);
        ss << std::setw(19) << int_to_hex(mapping->end_addr, 16);
        ss << std::setw(8) << prot_flags_to_str(mapping->perms);
        ss << mapping->info;
        if (m_i != mappings_sz - 1)
            ss << std::endl;
    }

    return ss.str();
}

void MemoryManager::insert_mapping(std::shared_ptr<ARION_MAPPING> mapping)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto mappings_it =
        std::lower_bound(this->mappings.begin(), this->mappings.end(), mapping,
                         [](const std::shared_ptr<ARION_MAPPING> a, const std::shared_ptr<ARION_MAPPING> b) {
                             return a->start_addr < b->start_addr;
                         });
    this->mappings.insert(mappings_it, mapping);

    arion->tracer->process_new_mapping(mapping);
}

void MemoryManager::remove_mapping(std::shared_ptr<ARION_MAPPING> mapping)
{
    auto mapping_it = std::find(this->mappings.begin(), this->mappings.end(), mapping);
    if (mapping_it == this->mappings.end())
        throw SegmentNotMappedException(mapping->start_addr, mapping->end_addr);
    this->mappings.erase(mapping_it);
}

void MemoryManager::merge_uc_mappings(ADDR start, ADDR end)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<ARION_MAPPING> mapping = this->get_mapping_at(start);
    uint32_t uc_perms = to_uc_perms(mapping->perms);

    size_t mapping_sz = end - start;
    std::vector<BYTE> data = this->read(start, mapping_sz);

    uc_err uc_unmap_err = uc_mem_unmap(arion->uc, start, mapping_sz);
    if (uc_unmap_err != UC_ERR_OK)
        throw UnicornUnmapException(uc_unmap_err);

    uc_err uc_map_err = uc_mem_map(arion->uc, start, mapping_sz, uc_perms);
    if (uc_map_err != UC_ERR_OK)
        throw UnicornMapException(uc_map_err);

    this->write(start, data.data(), mapping_sz);
}

void MemoryManager::merge_contiguous_uc_mappings()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uc_mem_region *regions;
    uint32_t regions_count;

    uc_err uc_regions_err = uc_mem_regions(arion->uc, &regions, &regions_count);
    if (uc_regions_err != UC_ERR_OK)
        throw UnicornMemRegionsException(uc_regions_err);

    if (!regions_count)
        return;

    ADDR start = regions[0].begin;
    ADDR end = regions[0].end + 1;
    uint32_t perms = regions[0].perms;
    bool merge = false;
    for (size_t region_i = 1; region_i < regions_count; region_i++)
    {
        if (regions[region_i].begin == end && regions[region_i].perms == perms)
        {
            merge = true;
            end = regions[region_i].end + 1;
        }
        else
        {
            if (merge)
                this->merge_uc_mappings(start, end);
            start = regions[region_i].begin;
            end = regions[region_i].end + 1;
            perms = regions[region_i].perms;
            merge = false;
        }
    }
    if (merge)
        this->merge_uc_mappings(start, end);
    free(regions);
}

ADDR MemoryManager::map(ADDR start_addr, size_t sz, PROT_FLAGS perms, std::string info)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    start_addr = align_up(start_addr);
    sz = align_up(sz);
    if (!this->can_map(start_addr, sz))
        throw MemAlreadyMappedException(start_addr, sz);

    uint32_t uc_perms = this->to_uc_perms(perms);
    uc_err uc_map_err = uc_mem_map(arion->uc, start_addr, sz, uc_perms);
    if (uc_map_err != UC_ERR_OK)
        throw UnicornMapException(uc_map_err);
    std::shared_ptr<ARION_MAPPING> mapping = std::make_unique<ARION_MAPPING>(start_addr, start_addr + sz, perms, info);

    this->insert_mapping(mapping);
    return start_addr;
}

ADDR MemoryManager::map_anywhere(ADDR addr, size_t sz, PROT_FLAGS perms, bool asc, std::string info)
{
    addr = this->align_up(addr);
    sz = this->align_up(sz);
    if (asc)
    {
        ADDR start_addr = addr;
        for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
        {
            ADDR end_addr = mapping->start_addr;
            if (end_addr < start_addr)
                continue;
            size_t available_space = end_addr - start_addr;
            if (sz <= available_space)
            {
                this->map(start_addr, sz, perms, info);
                return start_addr;
            }
            start_addr = mapping->end_addr;
        }
        return this->map(start_addr, sz, perms, info);
    }
    else
    {
        ADDR end_addr = addr;
        for (auto mapping_it = this->mappings.rbegin(); mapping_it != this->mappings.rend(); mapping_it++)
        {
            std::shared_ptr<ARION_MAPPING> mapping = *mapping_it;
            ADDR start_addr = mapping->end_addr;
            if (end_addr < start_addr)
                continue;
            size_t available_space = end_addr - start_addr;
            if (sz <= available_space)
            {
                return this->map(end_addr - sz, sz, perms, info);
            }
            end_addr = mapping->start_addr;
        }
        return this->map(end_addr - sz, sz, perms, info);
    }
}

ADDR MemoryManager::map_anywhere(size_t sz, PROT_FLAGS perms, bool asc, std::string info)
{
    return this->map_anywhere(0, sz, perms, asc, info);
}

void MemoryManager::unmap(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (start_addr > end_addr || start_addr > mapping->end_addr || end_addr <= mapping->start_addr)
        throw CantUnmapOutsideSegmentException(mapping->start_addr, mapping->end_addr, start_addr, end_addr);
    start_addr = std::max(start_addr, mapping->start_addr);
    end_addr = std::min(end_addr, mapping->end_addr);

    auto mapping_it = std::find(this->mappings.begin(), this->mappings.end(), mapping);
    if (mapping_it == this->mappings.end())
        throw SegmentNotMappedException(mapping->start_addr, mapping->end_addr);

    size_t mapping_del_sz = end_addr - start_addr;
    uc_err uc_unmap_err = uc_mem_unmap(arion->uc, start_addr, mapping_del_sz);
    if (uc_unmap_err != UC_ERR_OK)
        throw UnicornUnmapException(uc_unmap_err);
    this->mappings.erase(mapping_it);

    if (mapping->start_addr != start_addr)
    {
        std::shared_ptr<ARION_MAPPING> map_before =
            std::make_shared<ARION_MAPPING>(mapping->start_addr, start_addr, mapping->perms, mapping->info);
        this->insert_mapping(map_before);
    }
    if (mapping->end_addr != end_addr)
    {
        std::shared_ptr<ARION_MAPPING> map_after =
            std::make_shared<ARION_MAPPING>(end_addr, mapping->end_addr, mapping->perms, mapping->info);
        this->insert_mapping(map_after);
    }

    mapping.reset();
}

void MemoryManager::unmap(std::shared_ptr<ARION_MAPPING> mapping)
{
    this->unmap(mapping, mapping->start_addr, mapping->end_addr);
}

void MemoryManager::unmap(ADDR start_addr, ADDR end_addr)
{
    start_addr = align_up(start_addr);
    end_addr = align_up(end_addr);

    std::vector<std::shared_ptr<ARION_MAPPING>> mappings_cpy; // Need to clone to prevent concurrency
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
        mappings_cpy.push_back(mapping);
    for (std::shared_ptr<ARION_MAPPING> &mapping : mappings_cpy)
    {
        if (start_addr <= mapping->end_addr && end_addr > mapping->start_addr)
        {
            ADDR start_unmap = std::max(mapping->start_addr, start_addr);
            ADDR end_unmap = std::min(mapping->end_addr, end_addr);
            this->unmap(mapping, start_unmap, end_unmap);
        }
    }
}

void MemoryManager::unmap(ADDR seg_addr)
{
    seg_addr = align_up(seg_addr);
    std::shared_ptr<ARION_MAPPING> mapping = this->get_mapping_at(seg_addr);

    this->unmap(mapping);
}

void MemoryManager::unmap_all()
{
    std::vector<std::shared_ptr<ARION_MAPPING>> mappings_cpy; // Need to clone to prevent concurrency
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
        mappings_cpy.push_back(mapping);

    for (std::shared_ptr<ARION_MAPPING> &mapping : mappings_cpy)
        this->unmap(mapping);
}

void MemoryManager::protect(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr, PROT_FLAGS perms)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    start_addr = align_up(start_addr);
    end_addr = align_up(end_addr);

    start_addr = std::max(start_addr, mapping->start_addr);
    end_addr = std::min(end_addr, mapping->end_addr);

    ADDR old_start_addr = mapping->start_addr;
    ADDR old_end_addr = mapping->end_addr;
    PROT_FLAGS old_perms = mapping->perms;
    std::string old_info = mapping->info;
    uint32_t uc_perms = this->to_uc_perms(perms);
    mapping->start_addr = start_addr;
    mapping->end_addr = end_addr;
    if (start_addr == end_addr)
        this->remove_mapping(mapping);
    mapping->perms = perms;
    uc_err uc_protect_err = uc_mem_protect(arion->uc, start_addr, end_addr - start_addr, uc_perms);
    if (uc_protect_err != UC_ERR_OK)
        throw UnicornMemProtectException(uc_protect_err);

    if (old_start_addr != start_addr)
    {
        std::shared_ptr<ARION_MAPPING> map_before =
            std::make_shared<ARION_MAPPING>(old_start_addr, start_addr, old_perms, old_info);
        this->insert_mapping(map_before);
    }
    if (old_end_addr != end_addr)
    {
        std::shared_ptr<ARION_MAPPING> map_after =
            std::make_shared<ARION_MAPPING>(end_addr, old_end_addr, old_perms, old_info);
        this->insert_mapping(map_after);
    }
}

void MemoryManager::protect(std::shared_ptr<ARION_MAPPING> mapping, PROT_FLAGS perms)
{
    this->protect(mapping, mapping->start_addr, mapping->end_addr, perms);
}

void MemoryManager::protect(ADDR start_addr, ADDR end_addr, PROT_FLAGS perms)
{
    start_addr = align_up(start_addr);
    end_addr = align_up(end_addr);

    std::vector<std::shared_ptr<ARION_MAPPING>> mappings_cpy; // Need to clone to prevent concurrency
    for (std::shared_ptr<ARION_MAPPING> &mapping : this->mappings)
        mappings_cpy.push_back(mapping);
    for (std::shared_ptr<ARION_MAPPING> &mapping : mappings_cpy)
    {
        if (start_addr <= mapping->end_addr && end_addr > mapping->start_addr)
            this->protect(mapping, start_addr, end_addr, perms);
    }
}

void MemoryManager::protect(ADDR seg_addr, PROT_FLAGS perms)
{
    std::shared_ptr<ARION_MAPPING> mapping = this->get_mapping_at(seg_addr);
    this->protect(mapping, perms);
}

void MemoryManager::resize_mapping(std::shared_ptr<ARION_MAPPING> mapping, ADDR start_addr, ADDR end_addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    start_addr = align_up(start_addr);
    end_addr = align_up(end_addr);

    auto mapping_it = std::find(this->mappings.begin(), this->mappings.end(), mapping);
    if (mapping_it == this->mappings.end())
        throw SegmentNotMappedException(mapping->start_addr, mapping->end_addr);

    if (start_addr >= end_addr)
    {
        size_t mapping_del_sz = mapping->end_addr - mapping->start_addr;
        uc_err uc_unmap_err = uc_mem_unmap(arion->uc, mapping->start_addr, mapping_del_sz);
        if (uc_unmap_err != UC_ERR_OK)
            throw UnicornUnmapException(uc_unmap_err);
        this->mappings.erase(mapping_it);
        mapping.reset();
        return;
    }

    if (start_addr < mapping->start_addr)
    {
        size_t mapping_sz = mapping->start_addr - start_addr;
        uc_err uc_map_err = uc_mem_map(arion->uc, start_addr, mapping_sz, to_uc_perms(mapping->perms));
        if (uc_map_err != UC_ERR_OK)
            throw UnicornMapException(uc_map_err);
    }
    else if (start_addr > mapping->start_addr)
    {
        size_t mapping_del_sz = start_addr - mapping->start_addr;
        uc_err uc_unmap_err = uc_mem_unmap(arion->uc, mapping->start_addr, mapping_del_sz);
        if (uc_unmap_err != UC_ERR_OK)
            throw UnicornUnmapException(uc_unmap_err);
    }

    if (end_addr > mapping->end_addr)
    {
        size_t mapping_sz = end_addr - mapping->end_addr;
        uc_err uc_map_err = uc_mem_map(arion->uc, mapping->end_addr, mapping_sz, to_uc_perms(mapping->perms));
        if (uc_map_err != UC_ERR_OK)
            throw UnicornMapException(uc_map_err);
    }
    else
    {
        size_t mapping_del_sz = mapping->end_addr - end_addr;
        uc_err uc_unmap_err = uc_mem_unmap(arion->uc, end_addr, mapping_del_sz);
        if (uc_unmap_err != UC_ERR_OK)
            throw UnicornUnmapException(uc_unmap_err);
    }

    mapping->start_addr = start_addr;
    mapping->end_addr = end_addr;
}

void MemoryManager::resize_mapping(ADDR seg_addr, ADDR start_addr, ADDR end_addr)
{
    seg_addr = align_up(seg_addr);
    start_addr = align_up(start_addr);
    end_addr = align_up(end_addr);

    std::shared_ptr<ARION_MAPPING> mapping = this->get_mapping_at(seg_addr);
    resize_mapping(mapping, start_addr, end_addr);
}

std::vector<BYTE> MemoryManager::read(ADDR addr, size_t data_sz)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::vector<BYTE> read_data(data_sz);
    uc_err uc_read_err = uc_mem_read(arion->uc, addr, read_data.data(), read_data.size());
    if (uc_read_err != UC_ERR_OK)
        throw UnicornMemReadException(uc_read_err);
    return std::move(read_data);
}

uint64_t MemoryManager::read_val(ADDR addr, uint8_t n)
{
    std::vector<BYTE> read_data = this->read(addr, n);
    std::reverse(read_data.begin(), read_data.end());
    uint64_t res = 0;
    for (BYTE b : read_data)
    {
        res <<= 8;
        res |= b;
    }
    return res;
}

ADDR MemoryManager::read_ptr(ADDR addr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    return this->read_val(addr, arion->abi->get_attrs()->ptr_sz);
}

size_t MemoryManager::read_sz(ADDR addr)
{
    return this->read_ptr(addr);
}

int MemoryManager::read_fd(ADDR addr)
{
    return this->read_val(addr, ARION_FD_SZ);
}

std::string MemoryManager::read_hex(ADDR addr, size_t data_sz, char sep)
{
    std::vector<BYTE> read_data = this->read(addr, data_sz);

    size_t read_sz = read_data.size();
    std::stringstream hex_str;
    for (size_t b_i = 0; b_i < read_sz; b_i++)
    {
        BYTE b = read_data.at(b_i);
        hex_str << int_to_hex(b, 2, false);
        if (sep && b_i != read_sz - 1)
            hex_str << sep;
    }

    return hex_str.str();
}

std::string MemoryManager::read_ascii(ADDR addr, size_t data_sz)
{
    std::vector<BYTE> read_data = this->read(addr, data_sz);
    std::string ascii_str(read_data.begin(), read_data.end());
    return ascii_str;
}

std::string MemoryManager::read_c_string(ADDR addr)
{
    std::shared_ptr<ARION_MAPPING> mapping = this->get_mapping_at(addr);

    std::string c_str = "";
    ADDR curr_addr = addr;
    ADDR end_addr = mapping->end_addr;
    size_t buf_sz = ARION_BUF_SZ;
    while (curr_addr < end_addr)
    {
        if (end_addr - curr_addr < buf_sz)
            buf_sz = end_addr - curr_addr;
        std::vector<BYTE> read_data = this->read(curr_addr, buf_sz);
        for (BYTE b : read_data)
        {
            if (b)
            {
                c_str += (char)b;
            }
            else
                return c_str;
        }
        curr_addr += buf_sz;
    }
    return c_str;
}

std::vector<ADDR> MemoryManager::read_ptr_arr(ADDR addr)
{
    std::vector<ADDR> arr;
    ADDR curr_val = ARION_MAX_U64;
    off_t off = 0;
    while (curr_val)
    {
        curr_val = this->read_ptr(addr + off);
        if (curr_val)
            arr.push_back(curr_val);
        off += sizeof(ADDR);
    }
    return arr;
}

std::vector<cs_insn> MemoryManager::read_instrs(ADDR addr, size_t count)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::vector<cs_insn> instrs;
    off_t off = 0;
    BYTE *buf = (BYTE *)malloc(ARION_BUF_SZ);
    while (instrs.size() < count)
    {
        std::vector<BYTE> buf_vec = this->read(addr + off, ARION_BUF_SZ);
        memcpy(buf, buf_vec.data(), buf_vec.size());
        cs_insn *insn;
        size_t dis_count =
            cs_disasm(*arion->abi->curr_cs(), buf, ARION_BUF_SZ, addr + off, count - instrs.size(), &insn);
        for (uint64_t instr_i = 0; instr_i < dis_count && instr_i < count; instr_i++)
            instrs.push_back(insn[instr_i]);
        cs_free(insn, dis_count);
        off += ARION_BUF_SZ;
    }
    free(buf);
    return instrs;
}

void MemoryManager::write(ADDR addr, BYTE *data, size_t data_sz)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    uc_err uc_write_err = uc_mem_write(arion->uc, addr, data, data_sz);
    if (uc_write_err != UC_ERR_OK)
        throw UnicornMemWriteException(uc_write_err);
}

void MemoryManager::write_string(ADDR addr, std::string data)
{
    return this->write(addr, (BYTE *)data.c_str(), data.size() + 1);
}

void MemoryManager::write_val(ADDR addr, uint64_t val, uint8_t n)
{
    return this->write(addr, (BYTE *)&val, n);
}

void MemoryManager::write_ptr(ADDR addr, ADDR ptr)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    return this->write_val(addr, ptr, arion->abi->get_attrs()->ptr_sz);
}

void MemoryManager::write_sz(ADDR addr, size_t sz)
{
    return this->write_ptr(addr, sz);
}

void MemoryManager::write_fd(ADDR addr, int fd)
{
    return this->write_val(addr, fd, ARION_FD_SZ);
}

void MemoryManager::stack_push(uint64_t val)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    size_t ptr_sz = arion->abi->get_attrs()->ptr_sz;
    ADDR sp = arion->abi->read_arch_reg(arion->abi->get_attrs()->regs.sp);
    sp -= ptr_sz;
    arion->abi->write_arch_reg(arion->abi->get_attrs()->regs.sp, sp);
    this->write(sp, (BYTE *)&val, ptr_sz);
}

void MemoryManager::stack_push_bytes(BYTE *data, size_t data_sz)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR sp = arion->abi->read_arch_reg(arion->abi->get_attrs()->regs.sp);
    sp -= data_sz;
    arion->abi->write_arch_reg(arion->abi->get_attrs()->regs.sp, sp);
    this->write(sp, data, data_sz);
}

void MemoryManager::stack_push_string(std::string data)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR sp = arion->abi->read_arch_reg(arion->abi->get_attrs()->regs.sp);
    sp -= data.size() + 1;
    arion->abi->write_arch_reg(arion->abi->get_attrs()->regs.sp, sp);
    this->write_string(sp, data);
}

uint64_t MemoryManager::stack_pop()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR sp = arion->abi->read_arch_reg(arion->abi->get_attrs()->regs.sp);
    ADDR val = this->read_ptr(sp);
    sp += arion->abi->get_attrs()->ptr_sz;
    arion->abi->write_arch_reg(arion->abi->get_attrs()->regs.sp, sp);
    return val;
}

void MemoryManager::stack_align()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    ADDR sp = arion->abi->read_arch_reg(arion->abi->get_attrs()->regs.sp);
    sp -= (sp % arion->abi->get_attrs()->ptr_sz);
    arion->abi->write_arch_reg(arion->abi->get_attrs()->regs.sp, sp);
}

void MemoryManager::set_brk(ADDR brk)
{
    this->brk = brk;
}

ADDR MemoryManager::get_brk()
{
    return this->brk;
}

ADDR MemoryManager::align_up(ADDR addr)
{
    uint64_t delta = addr % this->page_sz;
    if (!delta)
        return addr;
    return addr + this->page_sz - delta;
}

uint32_t MemoryManager::to_uc_perms(PROT_FLAGS flags)
{
    uint32_t perms = 0;
    if (flags & 1)
        perms |= UC_PROT_EXEC;
    if (flags & 2)
        perms |= UC_PROT_WRITE;
    if (flags & 4)
        perms |= UC_PROT_READ;
    return perms;
}
