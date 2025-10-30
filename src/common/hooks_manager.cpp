#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/unicorn/unicorn.h>
#include <exception>
#include <memory>

using namespace arion;
using namespace arion_exception;

std::map<ARION_HOOK_TYPE, uc_hook_type> arion::ARION_UC_HOOK_TYPES{
    {ARION_HOOK_TYPE::INTR_HOOK, uc_hook_type::UC_HOOK_INTR},
    {ARION_HOOK_TYPE::INSN_HOOK, uc_hook_type::UC_HOOK_INSN},
    {ARION_HOOK_TYPE::CODE_HOOK, uc_hook_type::UC_HOOK_CODE},
    {ARION_HOOK_TYPE::BLOCK_HOOK, uc_hook_type::UC_HOOK_BLOCK},
    {ARION_HOOK_TYPE::MEM_READ_UNMAPPED_HOOK, uc_hook_type::UC_HOOK_MEM_READ_UNMAPPED},
    {ARION_HOOK_TYPE::MEM_WRITE_UNMAPPED_HOOK, uc_hook_type::UC_HOOK_MEM_WRITE_UNMAPPED},
    {ARION_HOOK_TYPE::MEM_FETCH_UNMAPPED_HOOK, uc_hook_type::UC_HOOK_MEM_FETCH_UNMAPPED},
    {ARION_HOOK_TYPE::MEM_READ_PROT_HOOK, uc_hook_type::UC_HOOK_MEM_READ_PROT},
    {ARION_HOOK_TYPE::MEM_WRITE_PROT_HOOK, uc_hook_type::UC_HOOK_MEM_WRITE_PROT},
    {ARION_HOOK_TYPE::MEM_FETCH_PROT_HOOK, uc_hook_type::UC_HOOK_MEM_FETCH_PROT},
    {ARION_HOOK_TYPE::MEM_READ_HOOK, uc_hook_type::UC_HOOK_MEM_READ},
    {ARION_HOOK_TYPE::MEM_WRITE_HOOK, uc_hook_type::UC_HOOK_MEM_WRITE},
    {ARION_HOOK_TYPE::MEM_FETCH_HOOK, uc_hook_type::UC_HOOK_MEM_FETCH},
    {ARION_HOOK_TYPE::MEM_READ_AFTER_HOOK, uc_hook_type::UC_HOOK_MEM_READ_AFTER},
    {ARION_HOOK_TYPE::INSN_INVALID_HOOK, uc_hook_type::UC_HOOK_INSN_INVALID},
    {ARION_HOOK_TYPE::EDGE_GENERATED_HOOK, uc_hook_type::UC_HOOK_EDGE_GENERATED},
    {ARION_HOOK_TYPE::TCG_OPCODE_HOOK, uc_hook_type::UC_HOOK_TCG_OPCODE},
    {ARION_HOOK_TYPE::TLB_FILL_HOOK, uc_hook_type::UC_HOOK_TLB_FILL}};

std::map<ARION_HOOK_TYPE, void *> arion::ARION_UC_HOOK_FUNCS{
    {ARION_HOOK_TYPE::INTR_HOOK, (void *)arion_intr_hook},
    {ARION_HOOK_TYPE::INSN_HOOK, (void *)arion_insn_hook},
    {ARION_HOOK_TYPE::CODE_HOOK, (void *)arion_code_hook},
    {ARION_HOOK_TYPE::BLOCK_HOOK, (void *)arion_block_hook},
    {ARION_HOOK_TYPE::MEM_READ_UNMAPPED_HOOK, (void *)arion_mem_read_unmapped_hook},
    {ARION_HOOK_TYPE::MEM_WRITE_UNMAPPED_HOOK, (void *)arion_mem_write_unmapped_hook},
    {ARION_HOOK_TYPE::MEM_FETCH_UNMAPPED_HOOK, (void *)arion_mem_fetch_unmapped_hook},
    {ARION_HOOK_TYPE::MEM_READ_PROT_HOOK, (void *)arion_mem_read_prot_hook},
    {ARION_HOOK_TYPE::MEM_WRITE_PROT_HOOK, (void *)arion_mem_write_prot_hook},
    {ARION_HOOK_TYPE::MEM_FETCH_PROT_HOOK, (void *)arion_mem_fetch_prot_hook},
    {ARION_HOOK_TYPE::MEM_READ_HOOK, (void *)arion_mem_read_hook},
    {ARION_HOOK_TYPE::MEM_WRITE_HOOK, (void *)arion_mem_write_hook},
    {ARION_HOOK_TYPE::MEM_FETCH_HOOK, (void *)arion_mem_fetch_hook},
    {ARION_HOOK_TYPE::MEM_READ_AFTER_HOOK, (void *)arion_mem_read_after_hook},
    {ARION_HOOK_TYPE::INSN_INVALID_HOOK, (void *)arion_insn_invalid_hook},
    {ARION_HOOK_TYPE::EDGE_GENERATED_HOOK, (void *)arion_edge_generated_hook},
    {ARION_HOOK_TYPE::TCG_OPCODE_HOOK, (void *)arion_tcg_opcode_hook},
    {ARION_HOOK_TYPE::TLB_FILL_HOOK, (void *)arion_tlb_fill_hook}};

void arion::arion_intr_hook(uc_engine *uc, uint32_t intno, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    U32_HOOK_CALLBACK arion_callback = std::get<U32_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        arion_callback(arion, intno, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
}

void arion::arion_insn_hook(uc_engine *uc, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    NO_PARAM_HOOK_CALLBACK arion_callback = std::get<NO_PARAM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        arion_callback(arion, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
}

void arion::arion_code_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    ADDR_SZ_HOOK_CALLBACK arion_callback = std::get<ADDR_SZ_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        arion_callback(arion, address, size, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
}

void arion::arion_block_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    ADDR_SZ_HOOK_CALLBACK arion_callback = std::get<ADDR_SZ_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        arion_callback(arion, address, size, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
}

bool arion::arion_mem_read_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                         void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_write_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                          void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_fetch_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                          void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_read_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                     void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_write_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                      void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_fetch_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                      void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_read_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_write_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                 void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_fetch_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                 void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_mem_read_after_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                      void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    MEM_HOOK_CALLBACK arion_callback = std::get<MEM_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, access, addr, size, val, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

bool arion::arion_insn_invalid_hook(uc_engine *uc, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    NO_PARAM_BOOL_HOOK_CALLBACK arion_callback = std::get<NO_PARAM_BOOL_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

void arion::arion_edge_generated_hook(uc_engine *uc, uc_tb *cur, uc_tb *prev, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    EDGE_HOOK_CALLBACK arion_callback = std::get<EDGE_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    arion_callback(arion, cur, prev, hook_param->user_data);
}

void arion::arion_tcg_opcode_hook(uc_engine *uc, uint64_t addr, uint64_t arg1, uint64_t arg2, int size, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    TCG_HOOK_CALLBACK arion_callback = std::get<TCG_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        arion_callback(arion, addr, arg1, arg2, size, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
}

bool arion::arion_tlb_fill_hook(uc_engine *uc, uint64_t addr, uc_mem_type type, uc_tlb_entry *result, void *user_data)
{
    ARION_HOOK_PARAM *hook_param = static_cast<ARION_HOOK_PARAM *>(user_data);
    TLB_HOOK_CALLBACK arion_callback = std::get<TLB_HOOK_CALLBACK>(hook_param->callback);
    std::shared_ptr<Arion> arion = hook_param->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    try
    {
        return arion_callback(arion, addr, type, result, hook_param->user_data);
    }
    catch (...)
    {
        arion->crash(std::current_exception());
    }
    return false;
}

std::unique_ptr<HooksManager> HooksManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::move(std::make_unique<HooksManager>(arion));
}

HooksManager::HooksManager(std::weak_ptr<Arion> arion) : arion(arion)
{
    // weak_ptr<Arion> will be expired when destructor is called, need to save Unicorn instance to unhook
    std::shared_ptr<Arion> arion_ = arion.lock();
    if (!arion_)
        throw ExpiredWeakPtrException("Arion");
    this->uc = arion_->uc;
}

HooksManager::~HooksManager()
{
    this->clear_hooks();
}

HOOK_ID HooksManager::gen_next_id()
{
    HOOK_ID hook_id;
    if (!this->free_hook_ids.empty())
    {
        hook_id = this->free_hook_ids.top();
        this->free_hook_ids.pop();
    }
    else
    {
        if (this->curr_id == ARION_MAX_U64)
            throw TooManyHooksException();
        hook_id = this->curr_id++;
    }
    return hook_id;
}

template <typename... UcParams>
HOOK_ID HooksManager::hook_uc(ARION_HOOK_TYPE type, HOOK_CALLBACK callback, void *user_data, ADDR start, ADDR end,
                              UcParams... uc_params)
{
    uc_hook_type uc_type = ARION_UC_HOOK_TYPES.at(type);
    void *arion_hook_func = ARION_UC_HOOK_FUNCS.at(type);
    uc_hook uc_id;

    struct ARION_HOOK_PARAM *param = new ARION_HOOK_PARAM(this->arion, callback, user_data);

    uc_err uc_add_err = uc_hook_add(this->uc, &uc_id, uc_type, arion_hook_func, param, start, end, uc_params...);
    if (uc_add_err != UC_ERR_OK)
        throw UnicornHookAddException(uc_add_err);

    std::shared_ptr<ARION_HOOK> arion_hook = std::make_shared<ARION_HOOK>(type, uc_id, param);
    HOOK_ID hook_id = this->gen_next_id();
    this->hooks[hook_id] = arion_hook;
    return hook_id;
}

HOOK_ID HooksManager::hook_intr(U32_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::INTR_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_insn(NO_PARAM_HOOK_CALLBACK callback, uint64_t insn, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::INSN_HOOK, callback, user_data, start, end, insn);
}

HOOK_ID HooksManager::hook_code(ADDR_SZ_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::CODE_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_addr(ADDR_SZ_HOOK_CALLBACK callback, arion::ADDR addr, void *user_data)
{
    return this->hook_code(callback, addr, addr, user_data);
}

HOOK_ID HooksManager::hook_block(ADDR_SZ_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::BLOCK_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_read_unmapped(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_READ_UNMAPPED_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_write_unmapped(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_WRITE_UNMAPPED_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_fetch_unmapped(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_FETCH_UNMAPPED_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_read_prot(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_READ_PROT_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_write_prot(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_WRITE_PROT_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_fetch_prot(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_FETCH_PROT_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_read(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_READ_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_write(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_WRITE_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_fetch(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_FETCH_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_mem_read_after(MEM_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::MEM_READ_AFTER_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_insn_invalid(NO_PARAM_BOOL_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::INSN_INVALID_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_edge_generated(EDGE_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::EDGE_GENERATED_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_tcg_opcode(TCG_HOOK_CALLBACK callback, uint64_t aux1, uint64_t aux2, ADDR start, ADDR end,
                                      void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::TCG_OPCODE_HOOK, callback, user_data, start, end, aux1, aux2);
}

HOOK_ID HooksManager::hook_tlb_fill(TLB_HOOK_CALLBACK callback, ADDR start, ADDR end, void *user_data)
{
    return this->hook_uc(ARION_HOOK_TYPE::TLB_FILL_HOOK, callback, user_data, start, end);
}

HOOK_ID HooksManager::hook_arion(ARION_HOOK_TYPE type, HOOK_CALLBACK callback, void *user_data)
{
    struct ARION_HOOK_PARAM *param = new ARION_HOOK_PARAM(this->arion, callback, user_data);
    std::shared_ptr<ARION_HOOK> arion_hook = std::make_shared<ARION_HOOK>(type, 0, param);
    HOOK_ID hook_id = this->gen_next_id();
    this->hooks[hook_id] = arion_hook;
    return hook_id;
}

HOOK_ID HooksManager::hook_fork(PROCESS_HOOK_CALLBACK callback, void *user_data)
{
    return this->hook_arion(ARION_HOOK_TYPE::FORK_HOOK, callback, user_data);
}

HOOK_ID HooksManager::hook_execve(PROCESS_HOOK_CALLBACK callback, void *user_data)
{
    return this->hook_arion(ARION_HOOK_TYPE::EXECVE_HOOK, callback, user_data);
}

HOOK_ID HooksManager::hook_syscall(SYSCALL_HOOK_CALLBACK callback, void *user_data)
{
    return this->hook_arion(ARION_HOOK_TYPE::SYSCALL_HOOK, callback, user_data);
}

void HooksManager::unhook(HOOK_ID hook_id)
{
    if (this->hooks.find(hook_id) == this->hooks.end())
        throw WrongHookIdException();
    std::shared_ptr<ARION_HOOK> arion_hook = this->hooks.at(hook_id);

    if (arion_hook->uc_id)
    {
        uc_err uc_del_err = uc_hook_del(this->uc, arion_hook->uc_id);
        if (uc_del_err != UC_ERR_OK)
            throw UnicornHookDelException(uc_del_err);
    }

    delete arion_hook->param;
    this->hooks.erase(hook_id);
    if (this->hooks.size())
        this->free_hook_ids.push(hook_id);
    else
    {
        this->free_hook_ids = std::stack<HOOK_ID>();
        this->curr_id = 1;
    }
}

void HooksManager::clear_hooks()
{
    std::vector<HOOK_ID> hook_ids; // need to clone keys to prevent concurrency editing
    for (const auto &hook_pair : this->hooks)
        hook_ids.push_back(hook_pair.first);
    for (HOOK_ID hook_id : hook_ids)
        this->unhook(hook_id);
}
