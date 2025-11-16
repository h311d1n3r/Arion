#include <arion/archs/arch_x86.hpp>
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

uint64_t arion::sys_clone(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    uint64_t clone_flags = params.at(0);
    ADDR new_sp = params.at(1);
    ADDR parent_tidptr = params.at(2);
    ADDR child_tidptr = params.at(3);
    ADDR new_tls = params.at(4);

    if (arion->arch->get_attrs()->arch != CPU_ARCH::X8664_ARCH)
    {
        ADDR tmp_child_tidptr = child_tidptr;
        child_tidptr = new_tls;
        new_tls = tmp_child_tidptr;
    }

    pid_t child_pid;
    if (clone_flags & CLONE_THREAD)
        child_pid = arion->threads->clone_thread(clone_flags, new_sp, new_tls, child_tidptr, parent_tidptr, 0);
    else
        child_pid = arion->threads->fork_process(clone_flags, new_sp, new_tls, child_tidptr, parent_tidptr, 0);
    arion->sync_threads();
    return child_pid;
}

uint64_t arion::sys_fork(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    pid_t child_pid = arion->threads->fork_process(0, 0, 0, 0, 0, 0);
    arion->sync_threads();
    return child_pid;
}

uint64_t arion::sys_execve(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
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
    return 0;
}

uint64_t arion::sys_exit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int error_code = params.at(0);

    REG pc_reg = arion->arch->get_attrs()->regs.pc;
    REG ret_reg = arion->arch->get_attrs()->syscalling_conv.ret_reg;
    ADDR exit_pc = arion->arch->read_arch_reg(pc_reg);
    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    if (arion_t->child_cleartid_addr)
    {
        arion->mem->write_val(arion_t->child_cleartid_addr, 0, sizeof(pid_t));
        arion->threads->futex_wake(arion_t->child_cleartid_addr, ARION_MAX_U32);
    }
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    arion->threads->remove_thread_entry(curr_tid);
    arion->sync_threads();
    if (!arion->arch->does_hook_intr())
    {
        size_t sys_instr_sz = arion->mem->read_instrs(exit_pc, 1).at(0).size;
        ADDR pc = arion->arch->read_arch_reg(pc_reg);
        arion->arch->write_reg(pc_reg, pc - sys_instr_sz);
    }
    return arion->arch->read_arch_reg(ret_reg);
}

uint64_t arion::sys_futex(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR uaddr = params.at(0);
    int op = params.at(1);
    uint32_t val = params.at(2);
    ADDR time_addr = params.at(3);
    ADDR uaddr2 = params.at(4);
    uint32_t val3 = params.at(5);

    // TODO: Use 32-bit time structure
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

uint64_t arion::sys_futex_time64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR uaddr = params.at(0);
    int op = params.at(1);
    uint32_t val = params.at(2);
    ADDR time_addr = params.at(3);
    ADDR uaddr2 = params.at(4);
    uint32_t val3 = params.at(5);

    // TODO: Use 64-bit time structure
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

uint64_t arion::sys_set_tid_address(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR tid_ptr = params.at(0);

    pid_t curr_tid = arion->threads->get_running_tid();
    std::unique_ptr<ARION_THREAD> arion_t = std::move(arion->threads->threads_map.at(curr_tid));
    arion_t->child_cleartid_addr = tid_ptr;
    arion->mem->write_val(tid_ptr, curr_tid, sizeof(pid_t));
    arion->threads->threads_map[curr_tid] = std::move(arion_t);
    return curr_tid;
}

uint64_t arion::sys_set_thread_area(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR u_info_addr = params.at(0);

    if (!u_info_addr)
        return EFAULT;

    ADDR new_tls = u_info_addr;
    if (arion->arch->get_attrs()->arch == CPU_ARCH::X86_ARCH)
        new_tls = static_cast<arion_x86::ArchManagerX86 *>(arion->arch.get())->new_tls(u_info_addr);

    arion->arch->load_tls(new_tls);

    return 0;
}

uint64_t arion::sys_set_tls(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR tls_addr = params.at(0);

    if (arion->arch->get_attrs()->arch == CPU_ARCH::X86_ARCH)
        tls_addr = static_cast<arion_x86::ArchManagerX86 *>(arion->arch.get())->new_tls(tls_addr);

    arion->arch->load_tls(tls_addr);

    return tls_addr;
}

uint64_t arion::sys_exit_group(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int error_code = params.at(0);

    REG pc_reg = arion->arch->get_attrs()->regs.pc;
    REG ret_reg = arion->arch->get_attrs()->syscalling_conv.ret_reg;
    ADDR pc = arion->arch->read_arch_reg(pc_reg);
    size_t sys_instr_sz = arion->mem->read_instrs(pc, 1).at(0).size;
    std::vector<pid_t> tids;
    for (auto &arion_t_entry : arion->threads->threads_map)
    {
        tids.push_back(arion_t_entry.first);
        pid_t tid = arion_t_entry.first;
        std::unique_ptr<ARION_THREAD> arion_t = std::move(arion_t_entry.second);
        if (arion_t->child_cleartid_addr)
        {
            arion->mem->write_val(arion_t->child_cleartid_addr, 0, sizeof(pid_t));
            arion->threads->futex_wake(arion_t->child_cleartid_addr, ARION_MAX_U32);
        }
        arion->threads->threads_map[tid] = std::move(arion_t);
    }
    for (pid_t tid : tids)
        arion->threads->remove_thread_entry(tid);
    arion->sync_threads();
    pc = arion->arch->read_arch_reg(pc_reg);
    arion->arch->write_reg(pc_reg, pc - sys_instr_sz);
    return arion->arch->read_arch_reg(ret_reg);
}

uint64_t arion::sys_clone3(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
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
        child_pid = arion->threads->fork_process(args->flags, args->stack + args->stack_size, args->tls,
                                                 args->child_tid, args->parent_tid, args->exit_signal);
    arion->sync_threads();
    return child_pid;
}

// obsolete in linux kernel > 6.X
uint64_t arion::sys_set_robust_list(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
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

uint64_t arion::sys_rseq(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
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
