#ifndef ARION_ARION_AFL_HPP
#define ARION_ARION_AFL_HPP

#include <arion/arion.hpp>
#include <arion/unicornafl/unicornafl.h>
#include <memory>
#include <sys/signal.h>

using ARION_AFL_INPUT_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, char *input, size_t input_sz,
                                                    uint32_t persistent_round, void *user_data)>;
using ARION_AFL_CRASH_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, uc_err res, char *input,
                                                    size_t input_sz, uint32_t persistent_round, void *user_data)>;

struct ARION_AFL_PARAM
{
    std::weak_ptr<Arion> arion;
    std::shared_ptr<ARION_CONTEXT> ctxt;
    bool keep_mem;
    ARION_AFL_INPUT_CALLBACK input_callback;
    ARION_AFL_CRASH_CALLBACK crash_callback;
    void *user_data;
    ARION_AFL_PARAM(std::weak_ptr<Arion> arion, std::shared_ptr<ARION_CONTEXT> ctxt, bool keep_mem,
                    ARION_AFL_INPUT_CALLBACK input_callback, ARION_AFL_CRASH_CALLBACK crash_callback, void *user_data)
        : arion(arion), ctxt(ctxt), keep_mem(keep_mem), input_callback(input_callback), crash_callback(crash_callback),
          user_data(user_data) {};
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
    unsigned char* input_file;
    static bool uc_input_callback(uc_engine *uc, char *input, size_t input_sz, uint32_t persistent_round,
                                  void *user_data);
    static bool uc_crash_callback(uc_engine *uc, uc_err res, char *input, int input_len, int persistent_round,
                                  void *user_data);

  public:
    ARION_EXPORT ArionAfl(std::weak_ptr<Arion> arion, unsigned char* input_file = nullptr) : arion(arion), input_file(input_file) {}
    void ARION_EXPORT fuzz(ARION_AFL_INPUT_CALLBACK input_callback, ARION_AFL_CRASH_CALLBACK crash_callback,
                           std::vector<arion::ADDR> exits, bool keep_mem = true,
                           std::vector<int> signals = {SIGSEGV, SIGABRT}, bool always_validate = false,
                           uint32_t persistent_iters = 1000, void *user_data = nullptr);
};

#endif // ARION_ARION_AFL_HPP
