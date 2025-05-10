#ifndef ARION_HOOKS_MANAGER_HPP
#define ARION_HOOKS_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <arion/unicorn/unicorn.h>
#include <variant>

class Arion; // forward declaration to prevent circular dependencies

using HOOK_ID = uint64_t;

using NO_PARAM_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, void *user_data)>;
using NO_PARAM_BOOL_HOOK_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, void *user_data)>;
using U32_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uint32_t val, void *user_data)>;
using ADDR_SZ_HOOK_CALLBACK =
    std::function<void(std::shared_ptr<Arion> arion, arion::ADDR addr, size_t sz, void *user_data)>;
using MEM_HOOK_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, uc_mem_type type, uint64_t addr, int size,
                                             int64_t val, void *user_data)>;
using EDGE_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uc_tb *cur, uc_tb *prev, void *user_data)>;
using TCG_HOOK_CALLBACK = std::function<void(std::shared_ptr<Arion> arion, uint64_t addr, uint64_t arg1, uint64_t arg2,
                                             int size, void *user_data)>;
using TLB_HOOK_CALLBACK = std::function<bool(std::shared_ptr<Arion> arion, uint64_t addr, uc_mem_type type,
                                             uc_tlb_entry *result, void *user_data)>;
using PROCESS_HOOK_CALLBACK =
    std::function<void(std::shared_ptr<Arion> arion, std::shared_ptr<Arion> child, void *user_data)>;
using HOOK_CALLBACK =
    std::variant<NO_PARAM_HOOK_CALLBACK, NO_PARAM_BOOL_HOOK_CALLBACK, U32_HOOK_CALLBACK, ADDR_SZ_HOOK_CALLBACK,
                 MEM_HOOK_CALLBACK, EDGE_HOOK_CALLBACK, TCG_HOOK_CALLBACK, TLB_HOOK_CALLBACK, PROCESS_HOOK_CALLBACK>;

enum ARION_HOOK_TYPE
{
    INTR_HOOK,
    INSN_HOOK,
    CODE_HOOK,
    BLOCK_HOOK,
    MEM_READ_UNMAPPED_HOOK,
    MEM_WRITE_UNMAPPED_HOOK,
    MEM_FETCH_UNMAPPED_HOOK,
    MEM_READ_PROT_HOOK,
    MEM_WRITE_PROT_HOOK,
    MEM_FETCH_PROT_HOOK,
    MEM_READ_HOOK,
    MEM_WRITE_HOOK,
    MEM_FETCH_HOOK,
    MEM_READ_AFTER_HOOK,
    INSN_INVALID_HOOK,
    EDGE_GENERATED_HOOK,
    TCG_OPCODE_HOOK,
    TLB_FILL_HOOK,
    FORK_HOOK,
    EXECVE_HOOK
};

extern std::map<ARION_HOOK_TYPE, uc_hook_type> ARION_UC_HOOK_TYPES;
extern std::map<ARION_HOOK_TYPE, void *> ARION_UC_HOOK_FUNCS;

struct ARION_HOOK_PARAM
{
    std::weak_ptr<Arion> arion;
    HOOK_CALLBACK callback;
    void *user_data;
    ARION_HOOK_PARAM(std::weak_ptr<Arion> arion, HOOK_CALLBACK callback, void *user_data)
        : arion(arion), callback(callback), user_data(user_data) {};
};

struct ARION_HOOK
{
    ARION_HOOK_TYPE type;
    uc_hook uc_id;
    ARION_HOOK_PARAM *param;
    ARION_HOOK(ARION_HOOK_TYPE type, uc_hook uc_id, ARION_HOOK_PARAM *param)
        : type(type), uc_id(uc_id), param(param) {};
};

void arion_intr_hook(uc_engine *uc, uint32_t intno, void *user_data);
void arion_insn_hook(uc_engine *uc, void *user_data);
void arion_code_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
void arion_block_hook(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
bool arion_mem_read_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                  void *user_data);
bool arion_mem_write_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                   void *user_data);
bool arion_mem_fetch_unmapped_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                                   void *user_data);
bool arion_mem_read_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
bool arion_mem_write_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                               void *user_data);
bool arion_mem_fetch_prot_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                               void *user_data);
bool arion_mem_read_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
bool arion_mem_write_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
bool arion_mem_fetch_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val, void *user_data);
bool arion_mem_read_after_hook(uc_engine *uc, uc_mem_type access, uint64_t addr, int size, int64_t val,
                               void *user_data);
bool arion_insn_invalid_hook(uc_engine *uc, void *user_data);
void arion_edge_generated_hook(uc_engine *uc, uc_tb *cur, uc_tb *prev, void *user_data);
void arion_tcg_opcode_hook(uc_engine *uc, uint64_t addr, uint64_t arg1, uint64_t arg2, int size, void *user_data);
bool arion_tlb_fill_hook(uc_engine *uc, uint64_t addr, uc_mem_type type, uc_tlb_entry *result, void *user_data);

class ARION_EXPORT HooksManager
{
  private:
    std::weak_ptr<Arion> arion;
    uc_engine *uc;
    HOOK_ID curr_id = 1;
    std::map<HOOK_ID, std::shared_ptr<ARION_HOOK>> hooks;
    std::stack<HOOK_ID> free_hook_ids;
    HOOK_ID gen_next_id();
    template <typename... UcParams>
    HOOK_ID hook_uc(ARION_HOOK_TYPE type, HOOK_CALLBACK callback, void *user_data, arion::ADDR start = 0,
                    arion::ADDR end = ARION_MAX_U64, UcParams... uc_params);
    HOOK_ID hook_arion(ARION_HOOK_TYPE type, HOOK_CALLBACK callback, void *user_data);

  public:
    HooksManager(std::weak_ptr<Arion> arion);
    ~HooksManager();
    static std::unique_ptr<HooksManager> initialize(std::weak_ptr<Arion> arion);
    HOOK_ID ARION_EXPORT hook_intr(U32_HOOK_CALLBACK callback, arion::ADDR start = 0, arion::ADDR end = ARION_MAX_U64,
                                   void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_insn(NO_PARAM_HOOK_CALLBACK callback, uint64_t insn, arion::ADDR start = 0,
                                   arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_code(ADDR_SZ_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                   arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_addr(ADDR_SZ_HOOK_CALLBACK callback, arion::ADDR addr, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_block(ADDR_SZ_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                    arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_read_unmapped(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                                arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_write_unmapped(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                                 arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_fetch_unmapped(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                                 arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_read_prot(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                            arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_write_prot(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                             arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_fetch_prot(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                             arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_read(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                       arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_write(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                        arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_fetch(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                        arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_mem_read_after(MEM_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                             arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_insn_invalid(NO_PARAM_BOOL_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                           arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_edge_generated(EDGE_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                             arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_tcg_opcode(TCG_HOOK_CALLBACK callback, uint64_t aux1, uint64_t aux2,
                                         arion::ADDR start = 0, arion::ADDR end = ARION_MAX_U64,
                                         void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_tlb_fill(TLB_HOOK_CALLBACK callback, arion::ADDR start = 0,
                                       arion::ADDR end = ARION_MAX_U64, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_fork(PROCESS_HOOK_CALLBACK callback, void *user_data = nullptr);
    HOOK_ID ARION_EXPORT hook_execve(PROCESS_HOOK_CALLBACK callback, void *user_data = nullptr);
    void ARION_EXPORT unhook(HOOK_ID hook_id);
    void ARION_EXPORT clear_hooks();

    template <typename... HookParams> void trigger_arion_hook(ARION_HOOK_TYPE type, HookParams... params)
    {
        for (auto &hook : this->hooks)
        {
            if (hook.second->type == type)
            {
                HOOK_CALLBACK &callback = hook.second->param->callback;
                std::shared_ptr<Arion> arion = hook.second->param->arion.lock();
                if (!arion)
                    throw ExpiredWeakPtrException("Arion");
                void *user_data = hook.second->param->user_data;

                std::visit(
                    [&](auto &&cb) {
                        using CallbackType = std::decay_t<decltype(cb)>;
                        if constexpr (std::is_invocable_v<CallbackType, std::shared_ptr<Arion>, HookParams..., void *>)
                            cb(arion, params..., user_data);
                        else
                            throw WrongHookParamsException();
                    },
                    callback);
            }
        }
    }
};

#endif // ARION_HOOKS_MANAGER_HPP
