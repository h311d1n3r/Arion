#include <arion/components/arion_afl.hpp>

bool ArionAfl::uc_input_callback(uc_engine *uc, char *input, size_t input_sz, uint32_t persistent_round,
                                 void *user_data)
{
    ARION_AFL_PARAM *hook_param = static_cast<ARION_AFL_PARAM *>(user_data);
    ARION_AFL_INPUT_CALLBACK arion_callback = hook_param->input_callback;
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    arion->context->restore(hook_param->ctxt, hook_param->keep_mem);
    try
    {
        return arion_callback(arion, input, input_sz, persistent_round, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool ArionAfl::uc_crash_callback(uc_engine *uc, uc_err res, char *input, int input_len, int persistent_round,
                                 void *user_data)
{
    ARION_AFL_PARAM *hook_param = static_cast<ARION_AFL_PARAM *>(user_data);
    ARION_AFL_CRASH_CALLBACK arion_callback = hook_param->crash_callback;
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");
    try
    {
        return arion_callback(arion, res, input, input_len, persistent_round, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

void ArionAfl::fuzz(ARION_AFL_INPUT_CALLBACK input_callback, ARION_AFL_CRASH_CALLBACK crash_callback,
                    std::vector<arion::ADDR> exits, bool keep_mem, std::vector<int> signals, bool always_validate,
                    uint32_t persistent_iters, void *user_data)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    if (!exits.size())
        throw UnicornAflNoExitsException();
    arion->init_afl_mode(signals);
    std::shared_ptr<ARION_CONTEXT> ctxt = arion->context->save();
    struct ARION_AFL_PARAM *param =
        new ARION_AFL_PARAM(this->arion, ctxt, keep_mem, input_callback, crash_callback, user_data);

    uc_afl_ret fuzz_ret;
    if (arion->baremetal) {
        fuzz_ret =
        uc_afl_fuzz(arion->uc, (char*)this->input_file , uc_input_callback, exits.data(),
                    exits.size(), uc_crash_callback, always_validate, persistent_iters, param);
    }
    else {
        fuzz_ret =
        uc_afl_fuzz(arion->uc, (char *)arion->get_program_args().at(0).c_str(), uc_input_callback, exits.data(),
                    exits.size(), uc_crash_callback, always_validate, persistent_iters, param);
    }
    if (fuzz_ret != UC_AFL_RET_OK)
    {
        auto err_it = UC_AFL_ERR_STR.find(fuzz_ret);
        std::string err_str = "Unknown UnicornAFL error.";
        if (err_it != UC_AFL_ERR_STR.end())
            err_str = err_it->second;
        throw UnicornAflException(err_str);
    }
    arion->stop_afl_mode();
}
