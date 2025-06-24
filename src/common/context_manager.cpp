#include <arion/arion.hpp>
#include <arion/common/context_manager.hpp>
#include <arion/common/global_excepts.hpp>
#include <fcntl.h>
#include <fstream>

using namespace arion;

std::unique_ptr<ContextManager> ContextManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::make_unique<ContextManager>(arion);
}

std::shared_ptr<ARION_CONTEXT> ContextManager::save()
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    pid_t running_tid = arion->threads->get_running_tid();
    std::vector<std::unique_ptr<ARION_THREAD>> thread_list;
    for (auto &arion_t : arion->threads->threads_map)
    {
        std::unique_ptr<ARION_THREAD> arion_t_cpy = std::make_unique<ARION_THREAD>(arion_t.second.get());
        if (arion_t_cpy->tid == running_tid)
        {
            arion_t_cpy->regs_state = arion->abi->dump_regs();
            arion_t_cpy->tls_addr = arion->abi->dump_tls();
        }
        thread_list.push_back(std::move(arion_t_cpy));
    }
    std::vector<std::unique_ptr<ARION_FUTEX>> futex_list;
    for (auto &arion_futex_vec : arion->threads->futex_list)
        for (std::unique_ptr<ARION_FUTEX> &arion_f : *arion_futex_vec.second)
            futex_list.push_back(std::make_unique<ARION_FUTEX>(arion_f.get()));
    std::vector<std::unique_ptr<ARION_MAPPING>> mapping_list;
    for (auto &arion_m : arion->mem->mappings)
    {
        std::unique_ptr<ARION_MAPPING> arion_m_cpy = std::make_unique<ARION_MAPPING>(arion_m.get());
        size_t mapping_sz = arion_m_cpy->end_addr - arion_m_cpy->start_addr;
        arion_m_cpy->saved_data = (BYTE *)malloc(mapping_sz);
        std::vector<BYTE> mapping_data = arion->mem->read(arion_m_cpy->start_addr, mapping_sz);
        memcpy(arion_m_cpy->saved_data, mapping_data.data(), mapping_sz);
        mapping_list.push_back(std::move(arion_m_cpy));
    }
    std::vector<std::unique_ptr<ARION_FILE>> file_list;
    for (auto &arion_f : arion->fs->files)
    {
        std::unique_ptr<ARION_FILE> arion_f_cpy = std::make_unique<ARION_FILE>(arion_f.second);
        off_t lseek_ret = lseek(arion_f_cpy->fd, 0, SEEK_CUR);
        if (lseek_ret > 0)
            arion_f_cpy->saved_off = lseek_ret;
        arion_f_cpy->fd = arion_f.first;
        file_list.push_back(std::move(arion_f_cpy));
    }
    std::vector<std::unique_ptr<ARION_SOCKET>> socket_list;
    for (auto &arion_s : arion->sock->sockets)
        socket_list.push_back(std::make_unique<ARION_SOCKET>(arion_s.second));
    return std::make_shared<ARION_CONTEXT>(running_tid, std::move(thread_list), std::move(futex_list),
                                           std::move(mapping_list), std::move(file_list), std::move(socket_list));
}

void ContextManager::restore(std::shared_ptr<ARION_CONTEXT> ctx, bool restore_mem)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::shared_ptr<ArionGroup> group = arion->get_group();
    pid_t pid = arion->get_pid();
    if (!group->has_arion_instance(pid) || group->get_arion_instance(pid) != arion)
        group->add_arion_instance(arion, pid, arion->get_pgid());

    for (auto arion_f_it = arion->fs->files.begin(); arion_f_it != arion->fs->files.end();)
    {
        auto &arion_f = *arion_f_it;
        if (arion_f.first > 2)
        {
            close(arion_f.first);
            arion_f_it = arion->fs->files.erase(arion_f_it);
        }
        else
            arion_f_it++;
    }
    for (std::unique_ptr<ARION_FILE> &arion_f : ctx->file_list)
    {
        int target_fd = arion_f->fd;
        if (target_fd <= 2)
            continue;
        arion_f->fd = open(arion_f->path.c_str(), arion_f->flags, arion_f->mode);
        if (arion_f->saved_off > 0)
            lseek(arion_f->fd, arion_f->saved_off, SEEK_SET);
        arion->fs->add_file_entry(target_fd, std::make_shared<ARION_FILE>(arion_f.get()));
    }
    for (auto &arion_s : arion->sock->sockets)
    {
        shutdown(arion_s.first, SHUT_RDWR);
        close(arion_s.first);
    }
    arion->sock->sockets.clear();
    for (std::unique_ptr<ARION_SOCKET> &arion_s : ctx->socket_list)
    {
        int target_fd = arion_s->fd;
        arion_s->fd = socket(arion_s->family, arion_s->type, arion_s->protocol);
        if (arion_s->s_addr && arion_s->s_addr_sz)
        {
            if (arion_s->server)
            {
                bind(arion_s->fd, arion_s->s_addr, arion_s->s_addr_sz);
                if (arion_s->server_listen)
                    listen(arion_s->fd, arion_s->server_backlog);
            }
            else
                connect(arion_s->fd, arion_s->s_addr, arion_s->s_addr_sz);
        }
        arion->sock->add_socket_entry(target_fd, std::make_shared<ARION_SOCKET>(arion_s.get()));
    }
    if (restore_mem)
    {
        arion->mem->unmap_all();
        for (std::unique_ptr<ARION_MAPPING> &arion_m : ctx->mapping_list)
        {
            size_t mapping_sz = arion_m->end_addr - arion_m->start_addr;
            arion->mem->map(arion_m->start_addr, mapping_sz, arion_m->perms, arion_m->info);
            if (arion_m->saved_data)
                arion->mem->write(arion_m->start_addr, arion_m->saved_data, mapping_sz);
        }
    }
    arion->threads->clear_threads();
    for (std::unique_ptr<ARION_THREAD> &arion_t : ctx->thread_list)
        arion->threads->threads_map[arion_t->tid] = std::make_unique<ARION_THREAD>(arion_t.get());
    for (std::unique_ptr<ARION_FUTEX> &arion_f : ctx->futex_list)
        arion->threads->futex_wait(arion_f->tid, arion_f->futex_addr, arion_f->futex_bitmask);
    arion->threads->set_running_tid(ctx->running_tid);
    arion->abi->load_regs(std::move(arion->threads->threads_map[ctx->running_tid]->regs_state));
    //never restore tls if arm_traps isn't loaded
    if (arion->baremetal) {
        if (!arion->baremetal->additional_mapped_segments.ARM_TRAPS) {return;}
    }
    arion->abi->load_tls(std::move(arion->threads->threads_map[ctx->running_tid]->tls_addr));
}

void ContextManager::save_to_file(std::string file_path)
{
    std::ofstream out_f(file_path, std::ios::binary);
    if (!out_f)
        throw FileOpenException(file_path);

    std::shared_ptr<ARION_CONTEXT> ctx = this->save();

    out_f.write(CONTEXT_FILE_MAGIC, strlen(CONTEXT_FILE_MAGIC));
    out_f.write((char *)&CONTEXT_FILE_VERSION, sizeof(float));

    out_f.write((char *)&ctx->running_tid, sizeof(pid_t));

    size_t threads_count = ctx->thread_list.size();
    out_f.write((char *)&threads_count, sizeof(size_t));
    for (std::unique_ptr<ARION_THREAD> &arion_t : ctx->thread_list)
    {
        std::vector<BYTE> srz_thread = serialize_arion_thread(arion_t.get());
        size_t srz_thread_sz = srz_thread.size();
        out_f.write((char *)&srz_thread_sz, sizeof(size_t));
        out_f.write((char *)srz_thread.data(), srz_thread_sz);
    }

    size_t futex_count = ctx->futex_list.size();
    out_f.write((char *)&futex_count, sizeof(size_t));
    for (std::unique_ptr<ARION_FUTEX> &arion_f : ctx->futex_list)
    {
        std::vector<BYTE> srz_futex = serialize_arion_futex(arion_f.get());
        size_t srz_futex_sz = srz_futex.size();
        out_f.write((char *)&srz_futex_sz, sizeof(size_t));
        out_f.write((char *)srz_futex.data(), srz_futex_sz);
    }

    size_t mappings_count = ctx->mapping_list.size();
    out_f.write((char *)&mappings_count, sizeof(size_t));
    for (std::unique_ptr<ARION_MAPPING> &arion_m : ctx->mapping_list)
    {
        std::vector<BYTE> srz_mapping = serialize_arion_mapping(arion_m.get());
        size_t srz_mapping_sz = srz_mapping.size();
        out_f.write((char *)&srz_mapping_sz, sizeof(size_t));
        out_f.write((char *)srz_mapping.data(), srz_mapping_sz);
    }

    size_t files_count = ctx->file_list.size();
    out_f.write((char *)&files_count, sizeof(size_t));
    for (std::unique_ptr<ARION_FILE> &arion_f : ctx->file_list)
    {
        std::vector<BYTE> srz_file = serialize_arion_file(arion_f.get());
        size_t srz_file_sz = srz_file.size();
        out_f.write((char *)&srz_file_sz, sizeof(size_t));
        out_f.write((char *)srz_file.data(), srz_file_sz);
    }

    size_t socket_count = ctx->socket_list.size();
    out_f.write((char *)&socket_count, sizeof(size_t));
    for (std::unique_ptr<ARION_SOCKET> &arion_s : ctx->socket_list)
    {
        std::vector<BYTE> srz_socket = serialize_arion_socket(arion_s.get());
        size_t srz_socket_sz = srz_socket.size();
        out_f.write((char *)&srz_socket_sz, sizeof(size_t));
        out_f.write((char *)srz_socket.data(), srz_socket_sz);
    }

    out_f.close();
}

void ContextManager::restore_from_file(std::string file_path)
{
    std::ifstream in_f(file_path, std::ios::binary);
    if (!in_f)
        throw FileOpenException(file_path);

    char read_magic[8];
    in_f.read(read_magic, sizeof(read_magic));
    if (strcmp(CONTEXT_FILE_MAGIC, read_magic))
        throw WrongContextFileMagicException(file_path);

    float read_ver;
    in_f.read((char *)&read_ver, sizeof(float));
    if (read_ver > CONTEXT_FILE_VERSION)
        throw NewerContextFileVersionException(file_path);

    std::shared_ptr<ARION_CONTEXT> ctx = std::make_shared<ARION_CONTEXT>();
    in_f.read((char *)&ctx->running_tid, sizeof(pid_t));
    size_t threads_count;
    in_f.read((char *)&threads_count, sizeof(size_t));
    for (size_t thread_i = 0; thread_i < threads_count; thread_i++)
    {
        size_t srz_thread_sz;
        in_f.read((char *)&srz_thread_sz, sizeof(size_t));
        std::vector<BYTE> srz_thread(srz_thread_sz);
        in_f.read((char *)srz_thread.data(), srz_thread_sz);
        ARION_THREAD *arion_t = deserialize_arion_thread(srz_thread);
        ctx->thread_list.push_back(std::make_unique<ARION_THREAD>(arion_t));
    }

    size_t futexes_count;
    in_f.read((char *)&futexes_count, sizeof(size_t));
    for (size_t futex_i = 0; futex_i < futexes_count; futex_i++)
    {
        size_t srz_futex_sz;
        in_f.read((char *)&srz_futex_sz, sizeof(size_t));
        std::vector<BYTE> srz_futex(srz_futex_sz);
        in_f.read((char *)srz_futex.data(), srz_futex_sz);
        ARION_FUTEX *arion_f = deserialize_arion_futex(srz_futex);
        ctx->futex_list.push_back(std::make_unique<ARION_FUTEX>(arion_f));
    }

    size_t mappings_count;
    in_f.read((char *)&mappings_count, sizeof(size_t));
    for (size_t mapping_i = 0; mapping_i < mappings_count; mapping_i++)
    {
        size_t srz_mapping_sz;
        in_f.read((char *)&srz_mapping_sz, sizeof(size_t));
        std::vector<BYTE> srz_mapping(srz_mapping_sz);
        in_f.read((char *)srz_mapping.data(), srz_mapping_sz);
        ARION_MAPPING *arion_m = deserialize_arion_mapping(srz_mapping);
        ctx->mapping_list.push_back(std::make_unique<ARION_MAPPING>(arion_m));
    }

    size_t files_count;
    in_f.read((char *)&files_count, sizeof(size_t));
    for (size_t file_i = 0; file_i < files_count; file_i++)
    {
        size_t srz_file_sz;
        in_f.read((char *)&srz_file_sz, sizeof(size_t));
        std::vector<BYTE> srz_file(srz_file_sz);
        in_f.read((char *)srz_file.data(), srz_file_sz);
        ARION_FILE *arion_f = deserialize_arion_file(srz_file);
        ctx->file_list.push_back(std::make_unique<ARION_FILE>(arion_f));
    }

    size_t sockets_count;
    in_f.read((char *)&sockets_count, sizeof(size_t));
    for (size_t socket_i = 0; socket_i < sockets_count; socket_i++)
    {
        size_t srz_socket_sz;
        in_f.read((char *)&srz_socket_sz, sizeof(size_t));
        std::vector<BYTE> srz_socket(srz_socket_sz);
        in_f.read((char *)srz_socket.data(), srz_socket_sz);
        ARION_SOCKET *arion_s = deserialize_arion_socket(srz_socket);
        ctx->socket_list.push_back(std::make_unique<ARION_SOCKET>(arion_s));
    }

    in_f.close();
    this->restore(ctx);
}
