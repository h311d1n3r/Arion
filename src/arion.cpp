#include <arion/arion.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/hooks_manager.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/common/config.hpp>
#include <arion/common/baremetal.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/baremetal_loader.hpp>
#include <arion/platforms/linux/elf_parser.hpp>
#include <arion/platforms/linux/lnx_syscall_manager.hpp>
#include <exception>
#include <filesystem>
#include <arion/keystone/keystone.h>
#include <memory>
#include <sys/wait.h>
#include <arion/unicorn/unicorn.h>
#include <variant>

using namespace arion;

pid_t Arion::NEXT_PID = ARION_PROCESS_PID;
std::map<pid_t, std::weak_ptr<Arion>> Arion::instances;

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

std::shared_ptr<Arion> Arion::new_instance(ProgramType program, std::string fs_path,
                                           std::vector<std::string> program_env, std::string cwd,
                                           std::unique_ptr<Config> config,
                                           pid_t pid)
{
    std::shared_ptr<Arion> arion = std::make_shared<Arion>();
    
    if (!pid)
    {
        arion->pid = Arion::NEXT_PID;
        arion->pgid = Arion::NEXT_PID;
        Arion::NEXT_PID++;
        Arion::instances[arion->pid] = arion;
    }
    else
    {
        arion->pid = pid;
        arion->pgid = pid;
    }

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

std::map<pid_t, std::weak_ptr<Arion>> Arion::get_arion_instances()
{
    return Arion::instances;
}

bool Arion::has_arion_instance(pid_t pid)
{
    return Arion::instances.find(pid) != Arion::instances.end();
}

std::weak_ptr<Arion> Arion::get_arion_instance(pid_t pid)
{
    auto instance_it = Arion::instances.find(pid);
    if (instance_it == Arion::instances.end())
        throw NoProcessWithPidException(pid);
    return instance_it->second;
}

Arion::~Arion()
{
    this->logger->debug("Destroying Arion instance.");
    this->children.clear();
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

bool Arion::run_current(bool multi_process, bool main_inst, std::optional<ADDR> start, std::optional<ADDR> end)
{
    if (!main_inst)
        this->run_children();
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
    if(start.has_value()) {
        pc_addr = start.value();
        this->abi->write_arch_reg(pc, pc_addr);
    } else pc_addr = this->abi->read_arch_reg(pc);
    uc_err uc_run_err =
        uc_emu_start(this->uc, pc_addr, end.value_or(0), 0, (multi_process || multi_thread) ? ARION_CYCLES_PER_THREAD : 0);
    pc_addr = this->abi->read_arch_reg(pc);
    if (this->uc_exception)
        std::rethrow_exception(this->uc_exception);
    if (uc_run_err != UC_ERR_OK && !this->sync)
        throw UnicornRunException(uc_run_err);
    if(this->hard_stop || pc_addr == end) {
        this->hard_stop = true;
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
    return true;
}

void Arion::run_children()
{
    std::vector<std::shared_ptr<Arion>> children_cpy;
    for (std::weak_ptr<Arion> child : this->children)
    {
        std::shared_ptr<Arion> child_ = child.lock();
        if (!child_)
            throw ExpiredWeakPtrException("Arion");
        children_cpy.push_back(child_);
    }
    for (std::shared_ptr<Arion> child : children_cpy)
    {
        if (child->is_zombie)
            continue;
        if (!child->run_current(true, false))
        {
            child->is_zombie = true;
            this->send_signal(child->pid, SIGCHLD);
        }
    }
}

void Arion::cleanup_process()
{
    for (std::weak_ptr<Arion> child : this->children)
    {
        std::shared_ptr<Arion> child_ = child.lock();
        if (!child_)
            throw ExpiredWeakPtrException("Arion");
        child_->cleanup_process();
    }
    this->children.clear();
    Arion::instances.erase(this->pid);
}

void Arion::run(std::optional<ADDR> start, std::optional<ADDR> end)
{
    if (!this->loader_params && this->baremetal->setup_memory) {
        throw ArionCustomBaremetalConfigurationNotSet();
    }
    this->hard_stop = false;
    this->running = true;
    if(!end.has_value()) {
        uc_err uc_ctl_err = uc_ctl(this->uc, UC_CTL_WRITE(UC_CTL_UC_USE_EXITS, 1), 1);
        if (uc_ctl_err != UC_ERR_OK)
            throw UnicornCtlException(uc_ctl_err);
    }

    bool first_round = true;
    while (true)
    {
        if (!this->run_current(this->children.size() > 0, true, first_round ? start : std::nullopt, end))
            break;
        this->run_children();
        if(first_round) first_round = false;
    }
    if(this->running)
        this->running = false;
    if(!this->hard_stop)
        this->cleanup_process();
}

void Arion::run_from(ADDR start) {
    this->run(start);
}

void Arion::run_to(ADDR end) {
    this->run(std::nullopt, end);
}

void Arion::stop(bool hard_stop)
{
    this->hard_stop = hard_stop;
    uc_err uc_stop_err = uc_emu_stop(this->uc);
    if (uc_stop_err != UC_ERR_OK)
        throw UnicornStopException(uc_stop_err);
}

void Arion::crash(std::exception_ptr exception)
{
    this->uc_exception = exception;
    this->stop(false);
}

void Arion::sync_threads()
{
    this->sync = true;
    this->stop(false);
}

std::shared_ptr<Arion> Arion::copy()
{
    std::shared_ptr<Arion> arion_cpy =
        Arion::new_instance(this->program_args, this->fs->get_fs_path(), this->program_env, this->fs->get_cwd_path(), 
                            std::make_unique<Config>(this->config->clone()));
    std::shared_ptr<ARION_CONTEXT> ctx = this->context->save();
    arion_cpy->context->restore(ctx);
    return arion_cpy;
}

bool Arion::is_running()
{
    return this->running;
}

void Arion::init_afl_mode(std::vector<int> signals) {
    this->afl_mode = true;
    this->afl_signals = signals;
}

void Arion::stop_afl_mode() {
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
    return this->children;
}

std::vector<std::shared_ptr<Arion>> Arion::get_pgid_children(pid_t pgid)
{
    std::vector<std::shared_ptr<Arion>> pgid_children;
    for (std::shared_ptr<Arion> child : this->children)
    {
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
    auto parent_it = Arion::instances.find(parent_pid);
    if (parent_it == Arion::instances.end())
        throw NoProcessWithPidException(parent_pid);
    std::shared_ptr<Arion> parent = parent_it->second.lock();
    if (!parent)
        throw ExpiredWeakPtrException("Arion");
    return parent->has_child(this->get_pid());
}

std::shared_ptr<Arion> Arion::get_child(pid_t child_pid)
{
    auto child_it =
        std::find_if(this->children.begin(), this->children.end(),
                     [child_pid](std::weak_ptr<Arion> it_child) { return it_child.lock()->pid == child_pid; });
    if (child_it == this->children.end())
        throw NoChildWithPidException(this->pid, child_pid);
    return *child_it;
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
    this->children.erase(
        std::remove_if(this->children.begin(), this->children.end(),
                       [child_pid](std::weak_ptr<Arion> child) { return child.lock()->pid == child_pid; }),
        this->children.end());
}

void Arion::execve(std::string file_path, std::vector<std::string> argv, std::vector<std::string> envp)
{
    if (argv.size())
        argv[0] = file_path;
    else
        argv.push_back(file_path);
    std::shared_ptr<Arion> new_inst = Arion::new_instance(argv, this->fs->get_fs_path(), envp, this->fs->get_cwd_path(),
                                                          std::make_unique<Config>(this->config->clone()), this->pid);
    std::shared_ptr<Arion> parent = this->parent.lock();
    if (!parent)
        throw ExpiredWeakPtrException("Arion");
    parent->replace_child(new_inst->pid, new_inst);
    new_inst->parent = this->parent;
    this->cleanup_process();
    Arion::instances[new_inst->pid] = new_inst;
    this->hooks->trigger_arion_hook(EXECVE_HOOK, new_inst);
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

void Arion::set_pgid(pid_t pgid)
{
    this->pgid = pgid;
}

void Arion::send_signal(pid_t source_pid, int signo)
{
    if(this->afl_mode && std::find(this->afl_signals.begin(), this->afl_signals.end(), signo) != this->afl_signals.end())
        abort(); // Will cause a crash in AFL instance
    std::shared_ptr<SIGNAL> sig = std::make_shared<SIGNAL>(source_pid, signo);
    this->pending_signals.push_back(sig);
}
