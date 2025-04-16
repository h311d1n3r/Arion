#include <arion/arion.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/common/config.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/common/config.hpp>
#include <arion/common/baremetal.hpp>
#include <arion/keystone/keystone.h>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/baremetal_loader.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_syscall_manager.hpp>
#include <arion/unicorn/unicorn.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <sys/wait.h>
#include <arion/unicorn/unicorn.h>
#include <variant>

using namespace arion;
using ProgramType = std::variant<std::vector<std::string>, std::unique_ptr<Baremetal>>;

std::map<arion::CPU_ARCH, std::pair<uc_arch, uc_mode>> arion::ARION_TO_UC_ARCH{
    {CPU_ARCH::X86_ARCH, {uc_arch::UC_ARCH_X86, uc_mode::UC_MODE_32}},
    {CPU_ARCH::X8664_ARCH, {uc_arch::UC_ARCH_X86, uc_mode::UC_MODE_64}},
    {CPU_ARCH::ARM_ARCH, {uc_arch::UC_ARCH_ARM, uc_mode::UC_MODE_ARM}},
    {CPU_ARCH::ARM64_ARCH, {uc_arch::UC_ARCH_ARM64, uc_mode::UC_MODE_LITTLE_ENDIAN}}};

std::map<arion::CPU_ARCH, std::vector<std::pair<ks_arch, ks_mode>>> arion::ARION_TO_KS_ARCH{
    {CPU_ARCH::X86_ARCH, {{ks_arch::KS_ARCH_X86, ks_mode::KS_MODE_32}}},
    {CPU_ARCH::X8664_ARCH, {{ks_arch::KS_ARCH_X86, ks_mode::KS_MODE_64}}},
    {CPU_ARCH::ARM_ARCH,
     {{ks_arch::KS_ARCH_ARM, ks_mode::KS_MODE_ARM}, {ks_arch::KS_ARCH_ARM, ks_mode::KS_MODE_THUMB}}},
    {CPU_ARCH::ARM64_ARCH, {{ks_arch::KS_ARCH_ARM64, ks_mode::KS_MODE_LITTLE_ENDIAN}}}};

std::map<arion::CPU_ARCH, std::vector<std::pair<cs_arch, cs_mode>>> arion::ARION_TO_CS_ARCH{
    {CPU_ARCH::X86_ARCH, {{cs_arch::CS_ARCH_X86, cs_mode::CS_MODE_32}}},
    {CPU_ARCH::X8664_ARCH, {{cs_arch::CS_ARCH_X86, cs_mode::CS_MODE_64}}},
    {CPU_ARCH::ARM_ARCH,
     {{cs_arch::CS_ARCH_ARM, cs_mode::CS_MODE_ARM}, {cs_arch::CS_ARCH_ARM, cs_mode::CS_MODE_THUMB}}},
    {CPU_ARCH::ARM64_ARCH, {{cs_arch::CS_ARCH_AARCH64, cs_mode::CS_MODE_LITTLE_ENDIAN}}}};

size_t ArionGroup::count_arion_instances()
{
    return this->instances.size();
}

std::map<pid_t, std::shared_ptr<Arion>> ArionGroup::get_arion_instances()
{
    return this->instances;
}

bool ArionGroup::has_arion_instance(pid_t pid)
{
    return this->instances.find(pid) != this->instances.end();
}

std::shared_ptr<Arion> ArionGroup::get_arion_instance(pid_t pid)
{
    auto instance_it = this->instances.find(pid);
    if (instance_it == this->instances.end())
        throw NoProcessWithPidException(pid);
    return instance_it->second;
}

void ArionGroup::add_arion_instance(std::shared_ptr<Arion> arion, std::optional<pid_t> pid, std::optional<pid_t> pgid)
{
    pid_t pid_val;
    if (!pid.has_value())
    {
        pid_val = this->next_pid;
        arion->set_pid(pid_val);
        arion->set_pgid(pgid.value_or(pid_val));
        this->next_pid++;
    }
    else
    {
        pid_val = pid.value();
        arion->set_pid(pid_val);
        arion->set_pgid(pgid.value_or(pid_val));
    }
    arion->set_group(shared_from_this());
    this->instances[pid_val] = arion;
}

void ArionGroup::remove_arion_instance(pid_t pid)
{
    std::shared_ptr<Arion> instance = this->get_arion_instance(pid);
    this->instances.erase(pid);
    instance->reset_group();
}

void ArionGroup::run()
{
    std::map<pid_t, std::shared_ptr<Arion>>::iterator instance_it;
    while ((instance_it = this->instances.begin()) != this->instances.end())
    {
        while (instance_it != this->instances.end())
        {
            auto weak_instance = *instance_it;
            std::shared_ptr<Arion> instance = weak_instance.second;
            if (!instance->is_zombie)
            {
                if (!instance->run_current())
                {
                    if (instance->has_parent())
                    {
                        std::shared_ptr<Arion> parent = instance->get_parent();
                        instance->is_zombie = true;
                        parent->send_signal(instance->get_pid(), SIGCHLD);
                    }
                    else
                    {
                        instance->clear_children();
                        instance_it = this->instances.erase(instance_it);
                        continue;
                    }
                }
                instance->set_run_start(std::nullopt);
            }
            else if (!instance->has_parent())
            {
                instance->clear_children();
                instance_it = this->instances.erase(instance_it);
                continue;
            }
            instance_it++;
        }
    }
}

void ArionGroup::stop_curr()
{
    std::shared_ptr instance = this->get_arion_instance(this->curr_pid);
    instance->stop();
}

pid_t ArionGroup::get_curr_pid()
{
    return this->curr_pid;
}

void ArionGroup::set_curr_pid(pid_t pid)
{
    this->curr_pid = pid;
}

pid_t ArionGroup::get_next_pid()
{
    return this->next_pid;
}

void ArionGroup::set_next_pid(pid_t pid)
{
    this->next_pid = pid;
}

std::shared_ptr<Arion> Arion::new_instance(ProgramType program, std::string fs_path,
                                           std::vector<std::string> program_env, std::string cwd,
                                           std::unique_ptr<Config> config)
{
    std::shared_ptr<Arion> arion = std::make_shared<Arion>();
    arion->config = std::move(config);

    arion->context = ContextManager::initialize(arion);
    arion->mem = MemoryManager::initialize(arion);
    arion->sock = SocketManager::initialize(arion);
    
    arion->fs = FileSystemManager::initialize(arion, fs_path, cwd);
    arion->logger = Logger::initialize(arion, arion->config->get_field<ARION_LOG_LEVEL>("log_lvl"));

    arion->program_env = program_env;

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            if (arg.empty())
                throw InvalidArgumentException("Program arguments must at least contain target name.");

            arion->program_args = arg;
            std::string program_path = arg.at(0);

            arion->logger->info("Initializing Arion instance for image \"" + program_path + "\".");

            if (!std::filesystem::exists(program_path))
                throw FileNotFoundException(program_path);
            if (!arion->fs->is_in_fs(program_path))
                throw FileNotInFsException(fs_path, program_path);

            std::shared_ptr<ElfParser> prog_parser = std::make_shared<ElfParser>(ElfParser(arion, program_path));
            prog_parser->process();
            arion->init_engines(prog_parser->arch);
            arion->hooks = HooksManager::initialize(arion);
            arion->threads = ThreadingManager::initialize(arion);
            arion->tracer = CodeTracer::initialize(arion);
            arion->init_program(prog_parser);
        }

        else if constexpr (std::is_same_v<T, std::unique_ptr<Baremetal>>) {
            arion->logger->info("Initializing Arion instance for Baremetal program.");
            arion->baremetal = std::move(arg);
            auto arch = arion->baremetal->arch;
            arion->init_engines(arch);

            auto code = arion->baremetal->coderaw;
            if(code->empty())
                throw std::runtime_error("Baremetal coderaw is empty!");

            arion->hooks = HooksManager::initialize(arion);
            arion->threads = ThreadingManager::initialize(arion);
            arion->tracer = CodeTracer::initialize(arion);

            arion->init_baremetal_program();
        }

    }, program);

    return arion;
}

Arion::~Arion()
{
    this->logger->debug("Destroying Arion instance.");

    std::shared_ptr<ArionGroup> group = this->group.lock();
    if (group && group->has_arion_instance(this->pid) && group->get_arion_instance(this->pid).get() == this)
        group->remove_arion_instance(this->pid);
    this->group.reset();

    this->parent.reset();
    this->syscalls.reset();
    this->abi.reset();
    this->tracer.reset();
    this->threads.reset();
    this->hooks.reset();
    this->sock.reset();
    this->mem.reset();
    this->context.reset();
    this->fs.reset();
    this->logger.reset();

    this->close_engines();
}

void Arion::init_engines(arion::CPU_ARCH arch)
{
    std::pair<uc_arch, uc_mode> uc_cpu_arch = ARION_TO_UC_ARCH[arch];
    uc_err uc_open_err = uc_open(uc_cpu_arch.first, uc_cpu_arch.second, &this->uc);
    if (uc_open_err != UC_ERR_OK)
        throw UnicornOpenException(uc_open_err);
    std::vector<std::pair<ks_arch, ks_mode>> ks_cpu_archs = ARION_TO_KS_ARCH[arch];
    for (std::pair<ks_arch, ks_mode> ks_cpu_arch : ks_cpu_archs)
    {
        ks_engine *ks;
        ks_err ks_open_err = ks_open(ks_cpu_arch.first, ks_cpu_arch.second, &ks);
        if (ks_open_err != KS_ERR_OK)
            throw KeystoneOpenException(ks_open_err);
        this->ks.push_back(ks);
    }
    std::vector<std::pair<cs_arch, cs_mode>> cs_cpu_archs = ARION_TO_CS_ARCH[arch];
    for (std::pair<cs_arch, cs_mode> cs_cpu_arch : cs_cpu_archs)
    {
        csh *cs = new csh();
        cs_err cs_open_err = cs_open(cs_cpu_arch.first, cs_cpu_arch.second, cs);
        if (cs_open_err != CS_ERR_OK)
            throw CapstoneOpenException(cs_open_err);
        this->cs.push_back(cs);
    }
}

void Arion::init_program(std::shared_ptr<ElfParser> prog_parser)
{
    std::shared_ptr<Arion> curr_instance = shared_from_this();
    const std::string program_path = this->program_args.at(0);
    this->abi = AbiManager::initialize(curr_instance, prog_parser->arch);
    if (prog_parser->arch == CPU_ARCH::X86_ARCH)
    {
        this->gdt_manager = GdtManager::initialize(curr_instance);
        this->gdt_manager->setup();
    }
    this->syscalls = LinuxSyscallManager::initialize(curr_instance); // must initialize AbiManager first
    switch (prog_parser->linkage)
    {
    case DYNAMIC_LINKAGE:
        init_dynamic_program(prog_parser);
        break;
    case STATIC_LINKAGE:
        init_static_program(prog_parser);
        break;
    default:
        throw UnknownLinkageTypeException(program_path);
    }
}

void Arion::init_baremetal_program()
{
    std::shared_ptr<Arion> curr_instance = shared_from_this();
    auto arch = this->baremetal->arch;
    this->abi = AbiManager::initialize(curr_instance, arch);
    if (arch == CPU_ARCH::X86_ARCH)
    {
        this->gdt_manager = GdtManager::initialize(curr_instance);
        this->gdt_manager->setup();
    }
    this->syscalls = LinuxSyscallManager::initialize(curr_instance); // must initialize AbiManager first
    if (!this->baremetal->setup_memory) {
        this->logger->info("No baremetal configuration. Using Default instance for baremetal");
        BaremetalLoader loader(curr_instance,this->program_env);
        this->loader_params = loader.process();
    }
}

void Arion::init_dynamic_program(std::shared_ptr<ElfParser> prog_parser)
{
    std::shared_ptr<Arion> curr_instance = shared_from_this();
    std::shared_ptr<ElfParser> interp_parser = std::make_shared<ElfParser>(curr_instance, prog_parser->interpreter);
    interp_parser->process();
    if (interp_parser->linkage != LINKAGE_TYPE::STATIC_LINKAGE)
        throw BadLinkageTypeException(prog_parser->interpreter);
    ElfLoader loader(curr_instance, interp_parser, prog_parser, this->program_args, this->program_env);
    this->loader_params = loader.process();
}

void Arion::init_static_program(std::shared_ptr<ElfParser> prog_parser)
{
    std::shared_ptr<Arion> curr_instance = shared_from_this();
    ElfLoader loader(curr_instance, prog_parser, this->program_args, this->program_env);
    this->loader_params = loader.process();
}

void Arion::close_engines()
{
    uc_close(this->uc);
    for (ks_engine *ks : this->ks)
        ks_close(ks);
    for (csh *cs : this->cs)
    {
        cs_close(cs);
        delete cs;
    }
}

bool Arion::run_current()
{
    std::shared_ptr<ArionGroup> group = this->group.lock();
    if (!group)
        throw ExpiredWeakPtrException("ArionGroup");

    group->set_curr_pid(this->pid);
    bool multi_process = group->count_arion_instances() > 1;
    this->running = true;
    size_t threads_count = this->threads->get_threads_count();
    if (!threads_count)
        return false;
    bool multi_thread = threads_count > 1;
    this->threads->handle_signals();
    if (this->threads->is_curr_locked())
    {
        this->threads->switch_to_next_thread();
        return true;
    }
    REG pc = this->abi->get_attrs()->regs.pc;
    ADDR pc_addr;
    if (this->start.has_value())
    {
        pc_addr = this->start.value();
        this->abi->write_arch_reg(pc, pc_addr);
    }
    else
        pc_addr = this->abi->read_arch_reg(pc);

    if (!this->end.has_value())
    {
        uc_err uc_ctl_err = uc_ctl(this->uc, UC_CTL_WRITE(UC_CTL_UC_USE_EXITS, 1), 1);
        if (uc_ctl_err != UC_ERR_OK)
            throw UnicornCtlException(uc_ctl_err);
    }
    uc_err uc_run_err = uc_emu_start(this->uc, pc_addr, this->end.value_or(0), 0,
                                     (multi_process || multi_thread) ? ARION_CYCLES_PER_THREAD : 0);
    pc_addr = this->abi->read_arch_reg(pc);
    if (this->uc_exception)
        std::rethrow_exception(this->uc_exception);
    if (uc_run_err != UC_ERR_OK && !this->sync)
        throw UnicornRunException(uc_run_err);
    if (this->end.has_value() && pc_addr == this->end.value())
    {
        this->end = 0;
        return false;
    }
    if (this->sync)
    {
        this->sync = false;
        return true;
    }
    threads_count = this->threads->get_threads_count();
    if (!threads_count)
        return false;
    this->threads->switch_to_next_thread();
    this->running = false;
    return true;
}

void Arion::set_run_bounds(std::optional<ADDR> start, std::optional<ADDR> end)
{
    this->start = start;
    this->end = end;
}

void Arion::set_run_start(std::optional<arion::ADDR> start)
{
    this->start = start;
}

void Arion::set_run_end(std::optional<arion::ADDR> end)
{
    this->end = end;
}

void Arion::stop()
{
    uc_err uc_stop_err = uc_emu_stop(this->uc);
    if (uc_stop_err != UC_ERR_OK)
        throw UnicornStopException(uc_stop_err);
}

void Arion::crash(std::exception_ptr exception)
{
    this->uc_exception = exception;
    this->stop();
}

void Arion::sync_threads()
{
    this->sync = true;
    this->stop();
}

std::shared_ptr<Arion> Arion::copy()
{
    std::shared_ptr<Arion> arion_cpy =
        Arion::new_instance(this->program_args, this->fs->get_fs_path(), this->program_env, this->fs->get_cwd_path(),
                            std::make_unique<Config>(this->config->clone()));
    std::shared_ptr<ArionGroup> group = this->group.lock();
    if (!group)
        throw ExpiredWeakPtrException("ArionGroup");
    group->add_arion_instance(arion_cpy, std::nullopt, this->get_pgid());
    std::shared_ptr<ARION_CONTEXT> ctx = this->context->save();
    arion_cpy->context->restore(ctx);
    return arion_cpy;
}

bool Arion::is_running()
{
    return this->running;
}

void Arion::init_afl_mode(std::vector<int> signals)
{
    this->afl_mode = true;
    this->afl_signals = signals;
}

void Arion::stop_afl_mode()
{
    this->afl_mode = false;
    this->afl_signals.clear();
}

bool Arion::has_parent()
{
    return !this->parent.expired();
}

std::shared_ptr<Arion> Arion::get_parent()
{
    std::shared_ptr<Arion> parent = this->parent.lock();
    if (!parent)
        throw ExpiredWeakPtrException("Arion");
    return parent;
}

std::vector<std::shared_ptr<Arion>> Arion::get_children()
{
    std::vector<std::shared_ptr<Arion>> children_vec;
    for (std::weak_ptr<Arion> child_weak : this->children)
    {
        std::shared_ptr<Arion> child = child_weak.lock();
        if (!child)
            throw ExpiredWeakPtrException("Arion");
        children_vec.push_back(child);
    }
    return children_vec;
}

std::vector<std::shared_ptr<Arion>> Arion::get_pgid_children(pid_t pgid)
{
    std::vector<std::shared_ptr<Arion>> pgid_children;
    for (std::weak_ptr<Arion> child_weak : this->children)
    {
        std::shared_ptr<Arion> child = child_weak.lock();
        if (!child)
            throw ExpiredWeakPtrException("Arion");
        if (child->get_pgid() == pgid)
            pgid_children.push_back(child);
    }
    return pgid_children;
}

pid_t Arion::add_child(std::shared_ptr<Arion> child)
{
    child->parent = this->shared_from_this();
    this->children.push_back(child);
    return child->pid;
}

bool Arion::has_child(pid_t child_pid)
{
    return std::find_if(this->children.begin(), this->children.end(), [child_pid](std::weak_ptr<Arion> it_child) {
               return it_child.lock()->pid == child_pid;
           }) != this->children.end();
}

bool Arion::is_child_of(pid_t parent_pid)
{
    std::shared_ptr<ArionGroup> group = this->group.lock();
    if (!group)
        throw ExpiredWeakPtrException("ArionGroup");
    std::shared_ptr<Arion> parent = group->get_arion_instance(parent_pid);
    return parent->has_child(this->get_pid());
}

std::shared_ptr<Arion> Arion::get_child(pid_t child_pid)
{
    auto child_weak_it =
        std::find_if(this->children.begin(), this->children.end(),
                     [child_pid](std::weak_ptr<Arion> it_child) { return it_child.lock()->pid == child_pid; });
    if (child_weak_it == this->children.end())
        throw NoChildWithPidException(this->pid, child_pid);
    std::shared_ptr<Arion> child = child_weak_it->lock();
    return child;
}

void Arion::replace_child(pid_t child_pid, std::shared_ptr<Arion> child)
{
    auto child_it =
        std::find_if(this->children.begin(), this->children.end(),
                     [child_pid](std::weak_ptr<Arion> it_child) { return it_child.lock()->pid == child_pid; });
    if (child_it == this->children.end())
        throw NoChildWithPidException(this->pid, child_pid);
    *child_it = child;
}

void Arion::remove_child(pid_t child_pid)
{
    this->children.erase(std::remove_if(this->children.begin(), this->children.end(),
                                        [child_pid](std::weak_ptr<Arion> child_weak) {
                                            std::shared_ptr<Arion> child = child_weak.lock();
                                            if (!child)
                                                throw ExpiredWeakPtrException("Arion");
                                            child->parent.reset();
                                            return child->pid == child_pid;
                                        }),
                         this->children.end());
}

void Arion::clear_children()
{
    for (std::weak_ptr<Arion> child_weak : this->children)
    {
        std::shared_ptr<Arion> child = child_weak.lock();
        if (!child)
            throw ExpiredWeakPtrException("Arion");
        child->parent.reset();
    }
    this->children.clear();
}

void Arion::execve(std::string file_path, std::vector<std::string> argv, std::vector<std::string> envp)
{
    if (argv.size())
        argv[0] = file_path;
    else
        argv.push_back(file_path);

    std::shared_ptr<ArionGroup> group = this->group.lock();
    if (!group)
        throw ExpiredWeakPtrException("ArionGroup");
    std::shared_ptr<Arion> new_inst = Arion::new_instance(argv, this->fs->get_fs_path(), envp, this->fs->get_cwd_path(),
                                                          std::make_unique<Config>(this->config->clone()));
    group->add_arion_instance(new_inst, this->get_pid(), this->get_pgid());
    std::shared_ptr<Arion> parent = this->parent.lock();
    if (parent)
    {
        parent->replace_child(new_inst->pid, new_inst);
        new_inst->parent = this->parent;
    }
    this->hooks->trigger_arion_hook(EXECVE_HOOK, new_inst);
    this->stop();
}

std::vector<std::string> Arion::get_program_args()
{
    return this->program_args;
}

std::vector<std::string> Arion::get_program_env()
{
    return this->program_env;
}

pid_t Arion::get_pid()
{
    return this->pid;
}

pid_t Arion::get_pgid()
{
    return this->pgid;
}

void Arion::set_pid(pid_t pid)
{
    this->pid = pid;
}

void Arion::set_pgid(pid_t pgid)
{
    this->pgid = pgid;
}

bool Arion::has_group()
{
    return !this->group.expired();
}

std::shared_ptr<ArionGroup> Arion::get_group()
{
    std::shared_ptr<ArionGroup> group = this->group.lock();
    if (!group)
        throw ExpiredWeakPtrException("ArionGroup");
    return group;
}

void Arion::set_group(std::shared_ptr<ArionGroup> group)
{
    this->group = group;
}

void Arion::reset_group()
{
    this->group.reset();
}

void Arion::send_signal(pid_t source_pid, int signo)
{
    if (this->afl_mode &&
        std::find(this->afl_signals.begin(), this->afl_signals.end(), signo) != this->afl_signals.end())
        abort(); // Will cause a crash in AFL instance
    std::shared_ptr<SIGNAL> sig = std::make_shared<SIGNAL>(source_pid, signo);
    this->pending_signals.push_back(sig);
}
