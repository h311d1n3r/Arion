#ifndef ARION_CODE_TRACER_HPP
#define ARION_CODE_TRACER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/common/memory_manager.hpp>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <string>

/// For a "light" TRACE_MODE, size at which the hits list should be flushed in the output trace file.
#define ARION_MAX_LIGHT_HITS 1024
/// For a "heavy" TRACE_MODE, size at which the hits list should be flushed in the output trace file.
#define ARION_MAX_HEAVY_HITS 64

namespace arion
{

class Arion;

/// Magic string for headers of Arion tracer files.
const char TRACER_FILE_MAGIC[] = "ARIONTRC";
/// Version number of Arion tracer file format.
const float TRACER_FILE_VERSION = 1.0;
/// Header size of Arion tracer files.
const uint16_t TRACER_FILE_HEADER_SIZE = 0x28;

/// These modes are used to configure the output file format.
enum ARION_EXPORT TRACE_MODE : uint8_t
{
    // Arion specific files
    INSTR, ///< Arion tracer file which stores the address of every executed instruction.
    CTXT,  ///< Arion tracer file which stores the CPU context of every executed instruction.
    BLOCK, ///< Arion tracer file which stores the address of every executed basic block (in control flow).

    // Non-Arion files
    DRCOV, ///< DrCov file format. Useful for compatibility with disassembler plugins.

    // Used to mark end of enum
    UNKNOWN ///< Unknown file format, should not be used.
};

/// Stores context information of a single hit. A hit can either be a new instruction or a new basic block depending on
/// the used TRACE_MODE.
struct ARION_EXPORT CODE_HIT
{
    /// Offset of the hit relative to the start address of its module.
    uint32_t off;
    /// Size of the hit. It can either be the instruction size or the basic block size depending on the used TRACE_MODE.
    uint16_t sz;
    /// ID of the hit module.
    uint16_t mod_id;
    /// CPU context as a map of register values. Only used in TRACE_MODE::CTXT.
    std::unique_ptr<std::map<REG, RVAL>> regs;

    /**
     * Builder for CODE_HIT instances.
     */
    CODE_HIT() {};
    /**
     * Builder for CODE_HIT instances.
     * @param[in] off Offset of the hit relative to the start address of its module.
     * @param[in] sz Size of the hit. It can either be the instruction size or the basic block size depending on the
     * used TRACE_MODE.
     * @param[in] mod_id ID of the hit module.
     * @param[in] regs CPU context as a map of register values. Only used in TRACE_MODE::CTXT.
     */
    CODE_HIT(uint32_t off, size_t sz, uint16_t mod_id) : off(off), sz(sz), mod_id(mod_id) {};
};

/// Stores general data over a memory mapping to be stored in the output trace file.
struct TRACER_MAPPING
{
    /// Start address of the memory mapping.
    ADDR start;
    /// End address of the memory mapping.
    ADDR end;
    /// Name associated with the memory mapping.
    std::string name;

    /*
     * Builder for TRACER_MAPPING instances.
     * @param[in] start Start address of the memory mapping.
     * @param[in] end End address of the memory mapping.
     * @param[in] name Name associated with the memory mapping.
     */
    TRACER_MAPPING(ADDR start, ADDR end, std::string name) : start(start), end(end), name(name) {};
};

/// This class is used to perform tracing operations over an Arion emulation and store the result in a dedicated file.
class ARION_EXPORT CodeTracer
{
  private:
    /// The Arion instance which emulation should be traced.
    std::weak_ptr<Arion> arion;
    /// True if the tracing is active.
    bool enabled = false;
    /// Trace mode, conditions the output file.
    TRACE_MODE mode;
    /// ID of the hook being triggered at every hit. The hits can either occur at every instruction or every basic block
    /// depending on the used TRACE_MODE.
    HOOK_ID curr_hook_id;
    /// Path to the output trace file.
    std::string out_f_path;
    /// Output stream of the output trace file.
    std::ofstream out_f;
    /// Total amount of hits to be stored in the output trace file.
    size_t total_hits;
    /// Offset in the output trace file to the total amount of hits.
    off_t total_hits_off;
    /// Offset in the output trace file to the modules section.
    off_t mod_sec_off;
    /// List of code hits (instructions or basic blocks) which have not yet been flushed in the output trace file.
    std::vector<std::unique_ptr<CODE_HIT>> hits;
    /// List of general data concerning memory mappings.
    std::vector<std::unique_ptr<TRACER_MAPPING>> mappings;
    /**
     * This hook is triggered at every instruction. Of course it is disabled when only basic blocks are traced.
     * @param[in] arion The Arion instance that produced the instruction hit.
     * @param[in] addr The address at which the instruction was hit.
     * @param[in] sz The size of the instruction that was hit.
     * @param[in] user_data Additional user data.
     */
    static void instr_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data);
    /**
     * This hook is triggered at every basic block. Of course it is disabled when every instructions are traced.
     * @param[in] arion The Arion instance that produced the block hit.
     * @param[in] addr The address at which the block was hit.
     * @param[in] sz The size of the block that was hit.
     * @param[in] user_data Additional user data.
     */
    static void block_hook(std::shared_ptr<Arion> arion, ADDR addr, size_t sz, void *user_data);
    /**
     * Creates and initializes the output trace file with already known data.
     */
    void prepare_file();
    /**
     * Writes remaining data in the output trace file and closes it.
     */
    void release_file();
    /**
     * Called at every hit (instruction or basic block). Stores the hit in the "hits" vector and flushes the whole in
     * the output trace file if necessary.
     * @param[in] addr Address of the hit.
     * @param[in] sz Size of the hit. It can either be the size of the hit instruction or basic block depending on the
     * used TRACE_MODE.
     */
    void process_hit(ADDR addr, size_t sz);
    /**
     * Flushes all hits in the "hits" vector into the output trace file.
     */
    void flush_hits();

  public:
    /**
     * Instanciates and initializes new CodeTracer objects with some parameters.
     * @param[in] arion The Arion instance which emulation should be traced.
     * @return A new CodeTracer instance.
     */
    static std::unique_ptr<CodeTracer> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for CodeTracer instances.
     * @param[in] arion The Arion instance which emulation should be traced.
     */
    CodeTracer(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Destructor for CodeTracer instances.
     */
    ~CodeTracer();
    /**
     * Starts tracing the emulation of the associated Arion instance.
     * @param[in] out_f_path Path to the output trace file.
     * @param[in] mode Trace mode, conditions the output file.
     */
    void ARION_EXPORT start(std::string out_f_path, TRACE_MODE mode);
    /**
     * Stops tracing the emulation of the associated Arion instance. Releases the output trace file.
     */
    void ARION_EXPORT stop();
    /**
     * Called every time a new memory region is mapped. Identifies if a new module is concerned and if so, stores its
     * general data for later use.
     * @param[in] mapping The newly allocated memory mapping.
     */
    void process_new_mapping(std::shared_ptr<ARION_MAPPING> mapping);
    /**
     * Checks whether the tracing is currently enabled in this instance.
     * @return True if the tracing is currently enabled in this instance.
     */
    bool ARION_EXPORT is_enabled();
    /**
     * Retrieves the current tracing mode for this instance.
     * @return The tracing mode.
     */
    TRACE_MODE ARION_EXPORT get_mode();
};

}; // namespace arion

#endif // ARION_CODE_TRACER_HPP
