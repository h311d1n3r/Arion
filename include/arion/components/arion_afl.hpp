#ifndef ARION_ARION_AFL_HPP
#define ARION_ARION_AFL_HPP

#include <arion/arion.hpp>
#include <arion/unicornafl/unicornafl.h>
#include <memory>
#include <sys/signal.h>

namespace arion
{

using ARION_AFL_INPUT_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, char *input, size_t input_sz,
                                                    uint32_t persistent_round, void *user_data)>;
using ARION_AFL_CRASH_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, uc_err res, char *input,
                                                    size_t input_sz, uint32_t persistent_round, void *user_data)>;

enum ARION_MEM_STRATEGY
{
    RECORD_EDITS,
    RESTORE_MAPPINGS,
    RAW_RESTORE,
    MANUAL_MANAGEMENT
};

struct ARION_AFL_PARAM
{
    std::weak_ptr<Arion> arion;
    std::shared_ptr<ARION_CONTEXT> ctxt;
    ARION_MEM_STRATEGY mem_strategy;
    ARION_AFL_INPUT_CALLBACK input_callback;
    ARION_AFL_CRASH_CALLBACK crash_callback;
    void *user_data;
    ARION_AFL_PARAM(std::weak_ptr<Arion> arion, std::shared_ptr<ARION_CONTEXT> ctxt, ARION_MEM_STRATEGY mem_strategy,
                    ARION_AFL_INPUT_CALLBACK input_callback, ARION_AFL_CRASH_CALLBACK crash_callback, void *user_data)
        : arion(arion), ctxt(ctxt), mem_strategy(mem_strategy), input_callback(input_callback),
          crash_callback(crash_callback), user_data(user_data) {};
};

inline std::map<uc_afl_ret, std::string> UC_AFL_ERR_STR = {
    {UC_AFL_RET_CHILD, "Fork worked. We are a child (no error)."},
    {UC_AFL_RET_NO_AFL, "No AFL, no need to fork (but no real error)."},
    {UC_AFL_RET_FINISHED, "We forked before but now AFL is gone (time to quit)."},
    {UC_AFL_RET_CALLED_TWICE, "Forkserver already running. This may be an error."},
    {UC_AFL_RET_ERROR, "Something went horribly wrong in the parent."}};

class ARION_EXPORT ArionAfl
{
  private:
    std::weak_ptr<Arion> arion;
    static bool uc_input_callback(uc_engine *uc, char *input, size_t input_sz, uint32_t persistent_round,
                                  void *user_data);
    static bool uc_crash_callback(uc_engine *uc, uc_err res, char *input, int input_len, int persistent_round,
                                  void *user_data);

  public:
    ARION_EXPORT ArionAfl(std::weak_ptr<Arion> arion) : arion(arion) {};
    void ARION_EXPORT fuzz(ARION_AFL_INPUT_CALLBACK input_callback, ARION_AFL_CRASH_CALLBACK crash_callback,
                           std::vector<ADDR> exits, ARION_MEM_STRATEGY mem_strategy = ARION_MEM_STRATEGY::RECORD_EDITS,
                           std::vector<int> signals = {SIGSEGV, SIGABRT}, bool always_validate = false,
                           uint32_t persistent_iters = 1000, void *user_data = nullptr);
};

}; // namespace arion

#endif // ARION_ARION_AFL_HPP
