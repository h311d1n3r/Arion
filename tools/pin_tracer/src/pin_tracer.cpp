#include <algorithm>
#include <arion/archs/abi_x86-64.hpp>
#include <arion/common/code_tracer.hpp>
#include <arion/crypto/md5.hpp>
#include <arion/utils/convert_utils.hpp>
#include <cstdlib>
#include <iostream>
#include <map>
#include <pin.H>
#include <random>
#include <string>

const std::string VERSION = "1.0-alpha";
const std::string TESTED_PIN_VERSION = "3.31";

// No support for FP0-FP7 registers
std::map<arion::REG, REG> ARION_PIN_REGS = {
    {UC_X86_REG_FS, REG_SEG_FS},
    {UC_X86_REG_GS, REG_SEG_GS},
    {UC_X86_REG_RAX, REG_RAX},
    {UC_X86_REG_RBP, REG_RBP},
    {UC_X86_REG_RBX, REG_RBX},
    {UC_X86_REG_RCX, REG_RCX},
    {UC_X86_REG_RDI, REG_RDI},
    {UC_X86_REG_RDX, REG_RDX},
    {UC_X86_REG_RIP, REG_RIP},
    {UC_X86_REG_RSI, REG_RSI},
    {UC_X86_REG_RSP, REG_RSP},
    {UC_X86_REG_SS, REG_SEG_SS},
    {UC_X86_REG_R8, REG_R8},
    {UC_X86_REG_R9, REG_R9},
    {UC_X86_REG_R10, REG_R10},
    {UC_X86_REG_R11, REG_R11},
    {UC_X86_REG_R12, REG_R12},
    {UC_X86_REG_R13, REG_R13},
    {UC_X86_REG_R14, REG_R14},
    {UC_X86_REG_R15, REG_R15},
    {UC_X86_REG_ST0, REG_ST0},
    {UC_X86_REG_ST1, REG_ST1},
    {UC_X86_REG_ST2, REG_ST2},
    {UC_X86_REG_ST3, REG_ST3},
    {UC_X86_REG_ST4, REG_ST4},
    {UC_X86_REG_ST5, REG_ST5},
    {UC_X86_REG_ST6, REG_ST6},
    {UC_X86_REG_ST7, REG_ST7},
    {UC_X86_REG_ZMM0, REG_YMM0}, // ZMM registers do not seem to be supported in Intel PIN yet
    {UC_X86_REG_ZMM1, REG_YMM1},
    {UC_X86_REG_ZMM2, REG_YMM2},
    {UC_X86_REG_ZMM3, REG_YMM3},
    {UC_X86_REG_ZMM4, REG_YMM4},
    {UC_X86_REG_ZMM5, REG_YMM5},
    {UC_X86_REG_ZMM6, REG_YMM6},
    {UC_X86_REG_ZMM7, REG_YMM7},
    {UC_X86_REG_ZMM8, REG_YMM8},
    {UC_X86_REG_ZMM9, REG_YMM9},
    {UC_X86_REG_ZMM10, REG_YMM10},
    {UC_X86_REG_ZMM11, REG_YMM11},
    {UC_X86_REG_ZMM12, REG_YMM12},
    {UC_X86_REG_ZMM13, REG_YMM13},
    {UC_X86_REG_ZMM14, REG_YMM14},
    {UC_X86_REG_ZMM15, REG_YMM15},
    {UC_X86_REG_FS_BASE, REG_SEG_FS_BASE},
    {UC_X86_REG_GS_BASE, REG_SEG_GS_BASE},
    {UC_X86_REG_RFLAGS, REG_RFLAGS},
};

std::map<TRACE_MODE, std::string> MODE_NAMES = {{TRACE_MODE::INSTR, "INSTR"},
                                                {TRACE_MODE::CTXT, "CTXT"},
                                                {TRACE_MODE::BLOCK, "BLOCK"},
                                                {TRACE_MODE::DRCOV, "DRCOV"}};

struct TRACE_VALUES
{
    int saved_stdin;
    int saved_stdout;
    int saved_stderr;
    std::string out_path;
    std::ofstream out_f;
    TRACE_MODE mode;
    size_t total_hits;
    off_t total_hits_off;
    off_t mod_sec_off;
    std::vector<std::unique_ptr<CODE_HIT>> hits;
    std::vector<std::unique_ptr<TRACER_MAPPING>> mappings;
};

std::string GenTmpPath()
{
    const std::string tmp_dir = "/tmp/";
    if (!std::filesystem::exists(tmp_dir))
        return "";

    std::string file_path;
    std::string file_name;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    do
    {
        std::ostringstream oss;
        for (int i = 0; i < 16; ++i)
        {
            oss << std::hex << dis(gen);
        }
        file_name = "pin_tracer-" + oss.str() + ".tmp";
        file_path = tmp_dir + file_name;
    } while (std::filesystem::exists(file_path));

    return file_path;
}

std::string MD5HashFile(std::string file_path)
{
    FILE *in_f = fopen(file_path.c_str(), "rb");
    if (!in_f)
        return std::string("");

    uint8_t *digest = (uint8_t *)malloc(MD5_DIGEST_LEN);
    md5_file(in_f, digest);

    std::stringstream hash_ss;
    for (size_t hash_i = 0; hash_i < MD5_DIGEST_LEN; hash_i++)
    {
        hash_ss << std::hex << std::setw(2) << std::setfill('0') << +digest[hash_i];
    }

    free(digest);
    fclose(in_f);

    return hash_ss.str();
}

std::map<arion::REG, arion::RVAL> DumpRegs(CONTEXT *ctxt)
{
    std::map<arion::REG, arion::RVAL> reg_vals = std::map<arion::REG, arion::RVAL>();
    for (arion::REG arion_reg : arion_x86_64::CTXT_REGS)
    {
        auto arion_pin_it = ARION_PIN_REGS.find(arion_reg);
        uint8_t reg_sz = arion_x86_64::ARCH_REGS_SZ[arion_reg];
        if (arion_pin_it != ARION_PIN_REGS.end())
        {
            if (reg_sz == 1)
                reg_vals[arion_reg].r8 = (arion::RVAL8)PIN_GetContextReg(ctxt, arion_pin_it->second);
            else if (reg_sz == 2)
                reg_vals[arion_reg].r16 = (arion::RVAL16)PIN_GetContextReg(ctxt, arion_pin_it->second);
            else if (reg_sz <= 4)
                reg_vals[arion_reg].r32 = (arion::RVAL32)PIN_GetContextReg(ctxt, arion_pin_it->second);
            else if (reg_sz <= 8)
                reg_vals[arion_reg].r64 = (arion::RVAL64)PIN_GetContextReg(ctxt, arion_pin_it->second);
            else if (reg_sz <= 16)
                PIN_GetContextRegval(ctxt, arion_pin_it->second, reg_vals[arion_reg].r128.data());
            else if (reg_sz <= 32)
                PIN_GetContextRegval(ctxt, arion_pin_it->second, reg_vals[arion_reg].r256.data());
            else
                PIN_GetContextRegval(ctxt, arion_pin_it->second, reg_vals[arion_reg].r512.data());
        }
        else
        {
            if (reg_sz == 1)
                reg_vals[arion_reg].r8 = (arion::RVAL8)0;
            else if (reg_sz == 2)
                reg_vals[arion_reg].r16 = (arion::RVAL16)0;
            else if (reg_sz <= 4)
                reg_vals[arion_reg].r32 = (arion::RVAL32)0;
            else if (reg_sz <= 8)
                reg_vals[arion_reg].r64 = (arion::RVAL64)0;
            else if (reg_sz <= 16)
                memset(reg_vals[arion_reg].r128.data(), 0, 16);
            else if (reg_sz <= 32)
                memset(reg_vals[arion_reg].r256.data(), 0, 32);
            else
                memset(reg_vals[arion_reg].r512.data(), 0, 64);
        }
    }
    return reg_vals;
}

VOID FlushHits(TRACE_VALUES *values)
{
    for (std::unique_ptr<CODE_HIT> &hit : values->hits)
    {
        values->out_f.write((char *)&hit->off, sizeof(uint32_t));
        values->out_f.write((char *)&hit->sz, sizeof(uint16_t));
        values->out_f.write((char *)&hit->mod_id, sizeof(uint16_t));
        if (values->mode == TRACE_MODE::CTXT)
            for (auto &reg : *hit->regs)
                values->out_f.write((char *)&reg.second, sizeof(arion::RVAL));
    }
    values->hits.clear();
}

VOID InstructionHit(CONTEXT *ctxt, UINT32 sz, TRACE_VALUES *values)
{
    ADDRINT ip = PIN_GetContextReg(ctxt, REG_INST_PTR);
    off_t mod_id = 0;
    arion::ADDR mapping_start = 0;
    bool found_mapping = false;
    for (std::unique_ptr<TRACER_MAPPING> &mapping : values->mappings)
    {
        if (ip >= mapping->start && ip < mapping->end)
        {
            mapping_start = mapping->start;
            found_mapping = true;
            break;
        }
        mod_id++;
    }
    if (!found_mapping)
        return;

    values->total_hits++;
    std::unique_ptr<CODE_HIT> hit = std::make_unique<CODE_HIT>(ip - mapping_start, sz, mod_id);
    if (values->mode == TRACE_MODE::CTXT)
        hit->regs = std::make_unique<std::map<arion::REG, arion::RVAL>>(DumpRegs(ctxt));
    values->hits.push_back(std::move(hit));

    size_t hits_sz = values->hits.size();
    size_t max_hits = values->mode == TRACE_MODE::CTXT ? MAX_HEAVY_HITS : MAX_LIGHT_HITS;
    if (hits_sz >= max_hits)
        FlushHits(values);
}

uint64_t leaf_val = 0;

VOID CPUIDHookBefore(CONTEXT *ctx)
{
    leaf_val = PIN_GetContextReg(ctx, REG_RAX);
}

VOID CPUIDHookAfter(CONTEXT *ctx)
{
    uint64_t rbx = PIN_GetContextReg(ctx, REG_RBX);
    uint64_t rcx = PIN_GetContextReg(ctx, REG_RCX);
    uint64_t rdx = PIN_GetContextReg(ctx, REG_RDX);

    uint64_t rbx_mask = 0xFFFFFFFFFFFFFFFF;
    uint64_t rcx_mask = 0xFFFFFFFFFFFFFFFF;
    uint64_t rdx_mask = 0xFFFFFFFFFFFFFFFF;

    switch (leaf_val)
    {
    case 1: {
        rcx_mask ^= 1;         // REMOVE SSE3 SUPPORT
        rcx_mask ^= (1 << 9);  // SSSE3 SUPPORT
        rcx_mask ^= (1 << 19); // SSE4.1 SUPPORT
        rcx_mask ^= (1 << 20); // SSE4.2 SUPPORT
        break;
    }
    case 7: {
        rbx_mask ^= (1 << 5); // REMOVE AVX2 SUPPORT
        rbx_mask ^= (1 << 9); // REMOVE ERMS SUPPORT
        break;
    }
    default:
        break;
    }

    PIN_SetContextReg(ctx, REG_RBX, rbx & rbx_mask);
    PIN_SetContextReg(ctx, REG_RCX, rcx & rcx_mask);
    PIN_SetContextReg(ctx, REG_RDX, rdx & rdx_mask);
    PIN_ExecuteAt(ctx);
}

VOID Instruction(INS ins, VOID *v)
{
    TRACE_VALUES *values = (TRACE_VALUES *)v;
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)InstructionHit, IARG_CONTEXT, IARG_UINT32, INS_Size(ins), IARG_PTR,
                   values, IARG_END);
}

VOID Cpuid(INS ins, VOID *v)
{
    if (INS_Opcode(ins) == XED_ICLASS_CPUID)
    {
        // Execute without some extensions (AVX2, SSSE3...) because they are not yet supported by QEMU
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CPUIDHookBefore, IARG_CONTEXT, IARG_END);
        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)CPUIDHookAfter, IARG_CONTEXT, IARG_END);
    }
}

VOID Block(TRACE trace, VOID *v)
{
    TRACE_VALUES *values = (TRACE_VALUES *)v;
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)InstructionHit, IARG_CONTEXT, IARG_UINT32, BBL_Size(bbl), IARG_PTR,
                       values, IARG_END);
    }
}

VOID ImageLoad(IMG img, VOID *v)
{
    TRACE_VALUES *values = (TRACE_VALUES *)v;
    std::string mapping_name = IMG_Name(img);
    arion::ADDR mapping_start = IMG_LowAddress(img);
    arion::ADDR mapping_end = IMG_HighAddress(img);
    if (!mapping_name.size() || !std::filesystem::exists(mapping_name))
        return;

    for (std::unique_ptr<TRACER_MAPPING> &tr_mapping : values->mappings)
    {
        if (mapping_name == tr_mapping->name)
        {
            if (mapping_start < tr_mapping->start)
                tr_mapping->start = mapping_start;
            if (mapping_end > tr_mapping->end)
                tr_mapping->end = mapping_end;
            return;
        }
    }

    values->mappings.push_back(std::make_unique<TRACER_MAPPING>(mapping_start, mapping_end, mapping_name));
}

void CopyAndRemoveFile(const std::string &src_path, const std::string &dest_path)
{
    std::ifstream src_f(src_path, std::ios::binary);
    if (!src_f)
        return;

    std::ofstream dest_f(dest_path, std::ios::binary);
    if (!dest_f)
        return;

    const std::size_t buf_sz = ARION_BUF_SZ;
    std::vector<char> buf(buf_sz);
    while (src_f)
    {
        src_f.read(buf.data(), buf_sz);
        dest_f.write(buf.data(), src_f.gcount());
    }

    src_f.close();
    dest_f.close();

    std::remove(src_path.c_str());
}

VOID ReleaseFile(TRACE_VALUES *values)
{
    switch (values->mode)
    {
    case TRACE_MODE::INSTR:
    case TRACE_MODE::CTXT:
    case TRACE_MODE::BLOCK: {
        off_t curr_pos = values->out_f.tellp();
        values->out_f.seekp(values->total_hits_off);
        values->out_f.write((char *)&values->total_hits, sizeof(size_t));
        values->out_f.seekp(values->mod_sec_off);
        values->out_f.write((char *)&curr_pos, sizeof(off_t));
        values->out_f.seekp(curr_pos);
        uint16_t mappings_sz = values->mappings.size();
        values->out_f.write((char *)&mappings_sz, sizeof(uint16_t));
        for (std::unique_ptr<TRACER_MAPPING> &mapping : values->mappings)
        {
            uint16_t mapping_name_len = mapping->name.length();
            values->out_f.write((char *)&mapping_name_len, sizeof(uint16_t));
            values->out_f.write(mapping->name.c_str(), mapping_name_len);
            values->out_f.write((char *)&mapping->start, sizeof(arion::ADDR));
            uint32_t mapping_len = mapping->end - mapping->start;
            values->out_f.write((char *)&mapping_len, sizeof(uint32_t));
            std::string mod_hash = MD5HashFile(mapping->name);
            uint16_t mod_hash_len = mod_hash.size();
            values->out_f.write((char *)&mod_hash_len, sizeof(uint16_t));
            values->out_f.write(mod_hash.c_str(), mod_hash_len);
        }
        break;
    }
    case TRACE_MODE::DRCOV: {
        std::string tmp_f_path = GenTmpPath();
        std::ofstream tmp_f(tmp_f_path, std::ios::binary);
        tmp_f << "DRCOV VERSION: 2" << std::endl;
        tmp_f << "DRCOV FLAVOR: drcov" << std::endl;
        tmp_f << "Module Table: version 2, count " << std::dec << +values->mappings.size() << std::endl;
        tmp_f << "Columns: id, base, end, entry, checksum, timestamp, path" << std::endl;
        off_t mod_id = 0;
        for (std::unique_ptr<TRACER_MAPPING> &mapping : values->mappings)
        {
            tmp_f << " " << std::dec << +mod_id << ", 0x" << std::hex << +mapping->start << ", 0x" << std::hex
                  << +mapping->end << ", 0x0, 0x0, 0x0, " << mapping->name << std::endl;
            mod_id++;
        }
        tmp_f << "BB Table: " << std::dec << +values->total_hits << " bbs" << std::endl;
        std::ifstream out_f(values->out_path, std::ios::binary);
        if (out_f.is_open())
        {
            size_t buf_sz = ARION_BUF_SZ;
            std::vector<char> buf(buf_sz);
            while (out_f)
            {
                out_f.read(buf.data(), buf_sz);
                tmp_f.write(buf.data(), out_f.gcount());
            }
            out_f.close();
        }
        tmp_f.close();
        CopyAndRemoveFile(tmp_f_path, values->out_path);
    }
    break;
    case TRACE_MODE::UNKNOWN:
    default:
        std::cerr << "Unknown trace mode." << std::endl;
        break;
    }
}

VOID Fini(INT32 code, VOID *v)
{
    TRACE_VALUES *values = (TRACE_VALUES *)v;
    dup2(values->saved_stdin, STDIN_FILENO);
    dup2(values->saved_stdout, STDOUT_FILENO);
    dup2(values->saved_stderr, STDERR_FILENO);
    std::cout << "Writing post-execution data..." << std::endl;
    FlushHits(values);
    ReleaseFile(values);
    if (values->out_f.is_open())
        values->out_f.close();
    std::cout << "Done." << std::endl;
}

std::string GetFormattedVersion(uint8_t len)
{
    std::string version = PIN_Version().substr(9);
    std::string fmt_version = "";

    for (char c : version)
    {
        if (c == '-')
            break;
        fmt_version += c;
    }

    for (uint8_t i = fmt_version.length(); i < len; i++)
        fmt_version += " ";

    return fmt_version;
}

VOID PrintBanner()
{
    std::cout << " ---------- PIN TRACER ----------" << std::endl;
    std::cout << "| Version: " << VERSION << "             |" << std::endl;
    std::cout << "| Tested PIN version: v" << TESTED_PIN_VERSION << "      |" << std::endl;
    std::cout << "| Current PIN version: v" << GetFormattedVersion(9) << "|" << std::endl;
    std::cout << "| Project: Arion                 |" << std::endl;
    std::cout << " --------------------------------" << std::endl;
    std::cout << std::endl;
}

VOID PrintHelp()
{
    std::cout << std::endl;
    std::cout << "Syntax: PIN_TRACER_PARAMS=\"MODE=mode;OUT=/path/to/output\" $PIN_ROOT/pin -t "
                 "libpin_tracer.so -- target_part"
              << std::endl;
    std::cout << "Where MODE is an integer from the following list :" << std::endl;
    std::cout << "0. INSTR - Trace address for all instructions" << std::endl;
    std::cout << "1. CTXT - Trace registers for all instructions" << std::endl;
    std::cout << "2. BLOCK - Trace address for all basic blocks" << std::endl;
    std::cout << "3. DRCOV - Generate drcov file" << std::endl;
    std::cout << std::endl;
}

std::map<std::string, std::string> ParseParams(std::string params_str)
{
    std::map<std::string, std::string> params;

    std::string key = "", value = "";
    bool key_mode = true;
    for (char c : params_str)
    {
        if (key_mode)
        {
            if (c == '=')
                key_mode = false;
            else if (c != ' ')
                key += c;
        }
        else
        {
            if (c == ';')
            {
                key_mode = true;
                if (key.size() && value.size())
                    params[key] = value;
                key = "";
                value = "";
            }
            else
                value += c;
        }
    }
    if (key.size() && value.size())
        params[key] = value;

    return params;
}

VOID PrepareFile(std::shared_ptr<TRACE_VALUES> values)
{
    switch (values->mode)
    {
    case TRACE_MODE::INSTR:
    case TRACE_MODE::CTXT:
    case TRACE_MODE::BLOCK: {
        // Start of Header
        values->out_f.write(TRACER_FILE_MAGIC, strlen(TRACER_FILE_MAGIC));
        values->out_f.write((char *)&TRACER_FILE_VERSION, sizeof(float));
        values->out_f.write((char *)&values->mode, sizeof(TRACE_MODE));
        values->total_hits_off = values->out_f.tellp();
        values->out_f.seekp(sizeof(size_t), std::ios::cur); // Reserved for later replacement of Total Hits
        uint8_t padding = 8 - (values->out_f.tellp() % 8);
        if (padding == 8)
            padding = 0;
        off_t secs_table_off = (off_t)values->out_f.tellp() + padding + sizeof(off_t);
        values->out_f.write((char *)&secs_table_off, sizeof(off_t));
        values->out_f.write((char *)&arion::EMPTY, padding);

        // Start of Sections Table
        values->mod_sec_off = values->out_f.tellp();
        values->out_f.seekp(sizeof(off_t), std::ios::cur); // Reserved for later replacement of Modules Section
        off_t regs_sec_off = 0;
        if (values->mode == TRACE_MODE::CTXT)
            regs_sec_off = (off_t)values->out_f.tellp() + sizeof(off_t) * 2;
        values->out_f.write((char *)&regs_sec_off, sizeof(off_t));
        /*std::vector<REG> ctxt_regs = arion->abi->get_context_regs();*/
        std::vector<arion::REG> ctxt_regs = arion_x86_64::CTXT_REGS;
        size_t ctxt_regs_sz = ctxt_regs.size();
        off_t data_sec_off =
            (off_t)values->out_f.tellp() + sizeof(off_t) + sizeof(size_t) + ctxt_regs_sz * sizeof(arion::REG);
        values->out_f.write((char *)&data_sec_off, sizeof(off_t));

        // Start of Registers Section
        values->out_f.write((char *)&ctxt_regs_sz, sizeof(size_t));
        for (arion::REG reg : ctxt_regs)
            values->out_f.write((char *)&reg, sizeof(arion::REG));
        break;
    }
    case TRACE_MODE::DRCOV:
    case TRACE_MODE::UNKNOWN:
        break;
    }
}

int main(int argc, char *argv[])
{
    PrintBanner();
    const char *params_str = std::getenv("PIN_TRACER_PARAMS");
    if (!params_str)
    {
        std::cerr << "Please define PIN_TRACER_PARAMS environment variable." << std::endl;
        PrintHelp();
        return 1;
    }
    std::map<std::string, std::string> params = ParseParams(std::string(params_str));
    std::shared_ptr<TRACE_VALUES> values = std::make_shared<TRACE_VALUES>();
    values->saved_stdin = dup(STDIN_FILENO);
    values->saved_stdout = dup(STDOUT_FILENO);
    values->saved_stderr = dup(STDERR_FILENO);

    auto mode_str_it = params.find("MODE");
    auto out_path_it = params.find("OUT");

    if (mode_str_it == params.end())
    {
        std::cerr << "Please define MODE parameter." << std::endl;
        PrintHelp();
        return 1;
    }
    std::string mode_str = mode_str_it->second;
    values->mode = (TRACE_MODE)atoi(mode_str.c_str());

    auto mode_name_it = MODE_NAMES.find(values->mode);
    if (mode_name_it == MODE_NAMES.end())
    {
        std::cerr << "Unknown trace mode." << std::endl;
        PrintHelp();
        return 1;
    }
    std::string mode_name = mode_name_it->second;

    values->out_path = "./out.trc";
    if (out_path_it != params.end())
        values->out_path = out_path_it->second;

    std::cout << "LAUNCH PARAMETERS:" << std::endl;
    std::cout << "- Mode: " << mode_name << std::endl;
    std::cout << "- Output: " << values->out_path << std::endl;
    std::cout << std::endl;

    values->out_f = std::ofstream(values->out_path, std::ios::binary | std::ios::out);
    if (!values->out_f.is_open())
    {
        std::cerr << "Couldn't open file \"" << values->out_path << "\" for writing." << std::endl;
        return 1;
    }

    std::cout << "Initializing trace file..." << std::endl;
    PrepareFile(values);

    std::cout << "Executing target..." << std::endl;
    int ret = 0;
    switch (values->mode)
    {
    case TRACE_MODE::INSTR:
        PIN_Init(argc, argv);
        INS_AddInstrumentFunction(Instruction, (void *)values.get());
        INS_AddInstrumentFunction(Cpuid, (void *)values.get());
        IMG_AddInstrumentFunction(ImageLoad, (void *)values.get());
        PIN_AddFiniFunction(Fini, (void *)values.get());
        PIN_StartProgram();
        break;
    case TRACE_MODE::CTXT:
        PIN_Init(argc, argv);
        INS_AddInstrumentFunction(Instruction, (void *)values.get());
        INS_AddInstrumentFunction(Cpuid, (void *)values.get());
        IMG_AddInstrumentFunction(ImageLoad, (void *)values.get());
        PIN_AddFiniFunction(Fini, (void *)values.get());
        PIN_StartProgram();
        break;
    case TRACE_MODE::BLOCK:
        PIN_Init(argc, argv);
        INS_AddInstrumentFunction(Cpuid, (void *)values.get());
        TRACE_AddInstrumentFunction(Block, (void *)values.get());
        IMG_AddInstrumentFunction(ImageLoad, (void *)values.get());
        PIN_AddFiniFunction(Fini, (void *)values.get());
        PIN_StartProgram();
        break;
    case TRACE_MODE::DRCOV:
        PIN_Init(argc, argv);
        INS_AddInstrumentFunction(Cpuid, (void *)values.get());
        TRACE_AddInstrumentFunction(Block, (void *)values.get());
        IMG_AddInstrumentFunction(ImageLoad, (void *)values.get());
        PIN_AddFiniFunction(Fini, (void *)values.get());
        PIN_StartProgram();
        break;
    default:
        std::cerr << "Unknown trace mode." << std::endl;
        PrintHelp();
        ret = 1;
        break;
    }

    if (values->out_f.is_open())
        values->out_f.close();

    return ret;
}
