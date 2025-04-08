#include <arion/archs/x86/gdt_manager.hpp>
#include <arion/arion.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/syscalls/process_syscalls.hpp>
#include <asm/ldt.h>
#include <linux/futex.h>
#include <linux/rseq.h>
#include <linux/sched.h>
#include <sched.h>

using namespace arion;

pid_t fork_process(std::shared_ptr<Arion> arion)
{
    CPU_ARCH arch = arion->abi->get_attrs()->arch;

    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    std::shared_ptr<Arion> forked_process = arion->copy();
    REG ret_reg = forked_process->abi->get_attrs()->syscalling_conv.ret_reg;
    forked_process->abi->write_arch_reg(ret_reg, 0);
    ADDR pc = arion->abi->read_arch_reg(pc_reg);
    ADDR next_pc;
    size_t sys_instr_sz;
    bool hooks_intr = arion->abi->does_hook_intr();
    // For architectures that hook with hook_intr, next_pc is already returned
    if (hooks_intr)
        next_pc = pc;
    else
    {
        sys_instr_sz = arion->mem->read_instrs(pc, 1).at(0).size;
        next_pc = pc + sys_instr_sz;
    }
    forked_process->abi->write_arch_reg(pc_reg, next_pc);
    pid_t forked_pid = arion->add_child(forked_process);
    arion->hooks->trigger_arion_hook(ARION_HOOK_TYPE::FORK_HOOK, forked_process);

    // Quick & dirty fix : coprocessor registers should be placed in context and better TLS management should be
    // provided
    if (arion->abi->get_attrs()->arch == CPU_ARCH::ARM64_ARCH)
    {
        uc_arm64_cp_reg tpidr = {0};
        tpidr.crn = 13;
        tpidr.crm = 0;
        tpidr.op0 = 3;
        tpidr.op1 = 3;
        tpidr.op2 = 2;
        uc_err uc_reg_err = uc_reg_read(arion->uc, UC_ARM64_REG_CP_REG, &tpidr); // TPIDR_EL0
        if (uc_reg_err != UC_ERR_OK)
            throw UnicornRegReadException(uc_reg_err);

        uc_reg_err = uc_reg_write(forked_process->uc, UC_ARM64_REG_CP_REG, &tpidr); // TPIDR_EL0
        if (uc_reg_err != UC_ERR_OK)
            throw UnicornRegWriteException(uc_reg_err);
    }

    if (!hooks_intr)
    {
        // If PC register was manually edited by user during fork hook, remove syscall instruction size
        ADDR user_edited_pc = arion->abi->read_arch_reg(pc_reg);
        if (user_edited_pc != pc)
            arion->abi->write_arch_reg(pc_reg, user_edited_pc - sys_instr_sz);
    }

    return forked_pid;
}

uint64_t sys_clone(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    uint64_t clone_flags = params.at(0);
    ADDR new_sp = params.at(1);
    ADDR parent_tidptr = params.at(2);
    ADDR child_tidptr = params.at(3);
    ADDR new_tls = params.at(4);

    if (arion->abi->get_attrs()->arch != CPU_ARCH::X8664_ARCH)
    {
        ADDR tmp_child_tidptr = child_tidptr;
        child_tidptr = new_tls;
        new_tls = tmp_child_tidptr;
    }

    pid_t child_pid;
    if (clone_flags & CLONE_THREAD)
        child_pid = arion->threads->clone_thread(clone_flags, new_sp, new_tls, child_tidptr, parent_tidptr, 0);
    else
        child_pid = fork_process(arion);
    arion->sync_threads();
    return child_pid;
}

uint64_t sys_fork(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t child_pid = fork_process(arion);
    arion->sync_threads();
    return child_pid;
}

uint64_t sys_execve(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR file_name_addr = params.at(0);
    ADDR argv_addr = params.at(1);
    ADDR envp_addr = params.at(2);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    std::vector<std::string> argv;
    if (argv_addr)
    {
        std::vector<ADDR> argv_arr = arion->mem->read_ptr_arr(argv_addr);
        for (ADDR addr : argv_arr)
            argv.push_back(arion->mem->read_c_string(addr));
    }
    std::vector<std::string> envp;
    if (envp_addr)
    {
        std::vector<ADDR> envp_arr = arion->mem->read_ptr_arr(envp_addr);
        for (ADDR addr : envp_arr)
            envp.push_back(arion->mem->read_c_string(addr));
    }
    arion->execve(file_name_fs, argv, envp);
    arion->stop(true);
    return 0;
}

uint64_t sys_exit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int error_code = params.at(0);

    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    ADDR pc = arion->abi->read_arch_reg(pc_reg);
    size_t sys_instr_sz = arion->mem->read_instrs(pc, 1).at(0).size;
    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    if (arion_t->flags & CLONE_CHILD_CLEARTID)
    {
        arion->mem->write_val(arion_t->child_tid_addr, 0, sizeof(pid_t));
        arion->threads->futex_wake(arion_t->child_tid_addr, ARION_MAX_U32);
    }
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    arion->threads->remove_thread_entry(curr_tid);
    arion->sync_threads();
    pc = arion->abi->read_arch_reg(pc_reg);
    arion->abi->write_reg(pc_reg, pc - sys_instr_sz);
    return arion->abi->read_arch_reg(ret_reg);
}

uint64_t sys_futex(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR uaddr = params.at(0);
    int op = params.at(1);
    uint32_t val = params.at(2);
    ADDR time_addr = params.at(3);
    ADDR uaddr2 = params.at(4);
    uint32_t val3 = params.at(5);

    uint64_t ret_val = 0;
    int masked_op = op & (FUTEX_PRIVATE_FLAG - 1);
    if (masked_op == FUTEX_WAIT || masked_op == FUTEX_WAIT_BITSET)
    {
        uint32_t uaddr_val = 0;
        if (uaddr)
            uaddr_val = arion->mem->read_val(uaddr, 4);
        if (uaddr_val != val)
            return EAGAIN;
        if (masked_op == FUTEX_WAIT)
            arion->threads->futex_wait_curr(uaddr, ARION_MAX_U32);
        else
            arion->threads->futex_wait_curr(uaddr, val3);
        ret_val = 0;
    }
    else if (masked_op == FUTEX_WAKE || masked_op == FUTEX_WAKE_BITSET)
    {
        if (masked_op == FUTEX_WAKE)
            ret_val = arion->threads->futex_wake(uaddr, ARION_MAX_U32);
        else
            ret_val = arion->threads->futex_wake(uaddr, val3);
    }
    arion->sync_threads();
    return ret_val;
}

uint64_t sys_set_tid_address(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR tid_ptr = params.at(0);

    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    arion_t->child_tid_addr = tid_ptr;
    arion->mem->write_val(tid_ptr, curr_tid, sizeof(pid_t));
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    return curr_tid;
}

uint64_t sys_set_thread_area(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR u_info_addr = params.at(0);

    if (!u_info_addr)
        return EFAULT;
    struct user_desc *u_info = (struct user_desc *)malloc(sizeof(struct user_desc));
    std::vector<BYTE> data = arion->mem->read(u_info_addr, sizeof(struct user_desc));
    memcpy(u_info, data.data(), data.size());

    if (arion->abi->get_attrs()->arch == CPU_ARCH::X86_ARCH)
    {
        if (u_info->entry_number == 0xFFFFFFFF)
            u_info->entry_number = arion->gdt_manager->find_free_idx(12);
        arion->gdt_manager->insert_entry(u_info->entry_number, u_info->base_addr, u_info->limit,
                                         ARION_A_PRESENT | ARION_A_DATA | ARION_A_DATA_WRITABLE | ARION_A_PRIV_3 |
                                             ARION_A_DIR_CON_BIT,
                                         ARION_F_PROT_32);
        if (!arion->mem->is_mapped(u_info->base_addr))
            arion->mem->map(u_info->base_addr,
                            u_info->limit_in_pages ? (u_info->limit * ARION_SYSTEM_PAGE_SZ) : u_info->limit, 0x6,
                            "[TLS]");
    }

    arion->mem->write(u_info_addr, (BYTE *)u_info, sizeof(struct user_desc));
    free(u_info);
    return 0;
}

uint64_t sys_set_tls(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR tls_addr = params.at(0);

    switch (arion->abi->get_attrs()->arch)
    {
    case CPU_ARCH::ARM_ARCH: {
        uc_arm_cp_reg cp15 = {0};
        cp15.cp = 15;
        cp15.is64 = 0;
        cp15.sec = 0;
        cp15.crn = 13;
        cp15.crm = 0;
        cp15.opc1 = 0;
        cp15.opc2 = 3;
        cp15.val = tls_addr;

        uc_err uc_reg_err = uc_reg_write(arion->uc, UC_ARM_REG_CP_REG, &cp15); // TPIDRURO
        if (uc_reg_err != UC_ERR_OK)
            throw UnicornRegWriteException(uc_reg_err);

        arion->mem->write_ptr(LINUX_32_ARM_GETTLS_ADDR + 0x10, tls_addr);
    }
    default:
        break;
    }

    return tls_addr;
}

uint64_t sys_exit_group(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int error_code = params.at(0);

    REG pc_reg = arion->abi->get_attrs()->regs.pc;
    REG ret_reg = arion->abi->get_attrs()->syscalling_conv.ret_reg;
    ADDR pc = arion->abi->read_arch_reg(pc_reg);
    size_t sys_instr_sz = arion->mem->read_instrs(pc, 1).at(0).size;
    std::vector<pid_t> tids;
    for (auto &arion_t_entry : arion->threads->threads_map)
    {
        tids.push_back(arion_t_entry.first);
        pid_t tid = arion_t_entry.first;
        std::unique_ptr<ARION_THREAD> arion_t = std::move(arion_t_entry.second);
        if (arion_t->flags & CLONE_CHILD_CLEARTID)
        {
            arion->mem->write_val(arion_t->child_tid_addr, 0, sizeof(pid_t));
            arion->threads->futex_wake(arion_t->child_tid_addr, ARION_MAX_U32);
        }
        arion->threads->threads_map[tid] = std::move(arion_t);
    }
    for (pid_t tid : tids)
        arion->threads->remove_thread_entry(tid);
    arion->sync_threads();
    pc = arion->abi->read_arch_reg(pc_reg);
    arion->abi->write_reg(pc_reg, pc - sys_instr_sz);
    return arion->abi->read_arch_reg(ret_reg);
}

uint64_t sys_clone3(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR args_addr = params.at(0);
    size_t args_sz = params.at(1);

    std::vector<BYTE> args_vec = arion->mem->read(args_addr, args_sz);
    struct clone_args *args = new struct clone_args;
    memcpy(args, args_vec.data(), args_sz);
    pid_t child_pid;
    if (args->flags & CLONE_THREAD)
        child_pid = arion->threads->clone_thread(args->flags, args->stack + args->stack_size, args->tls,
                                                 args->child_tid, args->parent_tid, args->exit_signal);
    else
        child_pid = fork_process(arion);
    arion->sync_threads();
    return child_pid;
}

// disapear in linux kernel > 6.X
uint64_t sys_set_robust_list(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR head = params.at(0);
    size_t len = params.at(1);

    if (len != sizeof(struct robust_list_head))
        return EINVAL;

    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    arion_t->robust_list_head = head;
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    return 0;
}

uint64_t sys_rseq(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR rseq_addr = params.at(0);
    uint32_t rseq_len = params.at(1);
    int rseq_flags = params.at(2);
    uint32_t rseq_sig = params.at(3);

    if (rseq_len != sizeof(struct rseq))
        return EINVAL;

    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    arion_t->rseq_addr = rseq_addr;
    arion_t->rseq_len = rseq_len;
    arion_t->rseq_sig = rseq_sig;
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    return 0;
}
