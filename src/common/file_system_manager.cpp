#include <arion/arion.hpp>
#include <arion/common/file_system_manager.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/platforms/linux/lnx_excepts.hpp>
#include <fcntl.h>
#include <filesystem>
#include <linux/limits.h>
#include <memory>
#include <regex>

using namespace arion;
using namespace arion_exception;

std::vector<BYTE> arion::serialize_arion_file(ARION_FILE *arion_f)
{
    std::vector<BYTE> srz_file;

    srz_file.insert(srz_file.end(), (BYTE *)&arion_f->fd, (BYTE *)&arion_f->fd + sizeof(int));
    size_t path_sz = arion_f->path.size();
    srz_file.insert(srz_file.end(), (BYTE *)&path_sz, (BYTE *)&path_sz + sizeof(size_t));
    srz_file.insert(srz_file.end(), (BYTE *)arion_f->path.c_str(), (BYTE *)arion_f->path.c_str() + path_sz);
    srz_file.insert(srz_file.end(), (BYTE *)&arion_f->flags, (BYTE *)&arion_f->flags + sizeof(int));
    srz_file.insert(srz_file.end(), (BYTE *)&arion_f->mode, (BYTE *)&arion_f->mode + sizeof(mode_t));
    srz_file.insert(srz_file.end(), (BYTE *)&arion_f->blocking, (BYTE *)&arion_f->blocking + sizeof(bool));
    srz_file.insert(srz_file.end(), (BYTE *)&arion_f->saved_off, (BYTE *)&arion_f->saved_off + sizeof(off_t));

    return srz_file;
}

ARION_FILE *arion::deserialize_arion_file(std::vector<BYTE> srz_file)
{
    ARION_FILE *arion_f = new ARION_FILE;

    off_t off = 0;
    memcpy(&arion_f->fd, srz_file.data() + off, sizeof(int));
    off += sizeof(int);
    size_t path_sz;
    memcpy(&path_sz, srz_file.data() + off, sizeof(size_t));
    off += sizeof(size_t);
    char *path = (char *)malloc(path_sz);
    memcpy(path, srz_file.data() + off, path_sz);
    arion_f->path = std::string(path, path_sz);
    free(path);
    off += path_sz;
    memcpy(&arion_f->flags, srz_file.data() + off, sizeof(int));
    off += sizeof(int);
    memcpy(&arion_f->mode, srz_file.data() + off, sizeof(mode_t));
    off += sizeof(mode_t);
    memcpy(&arion_f->blocking, srz_file.data() + off, sizeof(bool));
    off += sizeof(bool);
    memcpy(&arion_f->saved_off, srz_file.data() + off, sizeof(off_t));

    return arion_f;
}

std::unique_ptr<ProcFSManager> ProcFSManager::initialize(std::weak_ptr<Arion> arion)
{
    std::unique_ptr<ProcFSManager> procfs = std::make_unique<ProcFSManager>(arion);
    return std::move(procfs);
}

bool ProcFSManager::is_procfs_path(std::string path)
{
    return std::regex_match(path, ProcFSManager::PROCFS_REGEX);
}

std::string ProcFSManager::convert(std::string path)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    std::smatch match;
    if (!std::regex_match(path, match, ProcFSManager::PROCFS_REGEX))
        return "";
    pid_t pid = 0;
    if (match[1] == "self")
        pid = arion->get_pid();
    else
        pid = stoi(match[1]);
    std::shared_ptr group = arion->get_group();
    if (!group->has_arion_instance(pid))
        return "";
    std::shared_ptr<Arion> proc_inst = group->get_arion_instance(pid);
    if (match[2] == "exe")
    {
        std::vector<std::string> args = proc_inst->get_program_args();
        if (!args.size())
            return "";
        return args.at(0);
    }
    return "";
}

std::unique_ptr<FileSystemManager> FileSystemManager::initialize(std::weak_ptr<Arion> arion, std::string fs_path,
                                                                 std::string cwd_path)
{
    std::unique_ptr<FileSystemManager> fs = std::make_unique<FileSystemManager>(arion, fs_path, cwd_path);
    fs->procfs = std::move(ProcFSManager::initialize(arion));
    std::vector<std::string> stdio_paths =
        std::vector<std::string>({std::string("/dev/stdin"), std::string("/dev/stdout"), std::string("/dev/stderr")});
    for (uint8_t stdio_i = 0; stdio_i < stdio_paths.size(); stdio_i++)
    {
        try
        {
            std::string found_path = FileSystemManager::find_fd_path(stdio_i);
            if (found_path.size())
                stdio_paths[stdio_i] = found_path;
        }
        catch (std::exception e)
        {
        };
    }
    fs->add_file_entry(0, std::make_shared<ARION_FILE>(0, stdio_paths.at(0), O_RDONLY, 0), false);
    fs->add_file_entry(1, std::make_shared<ARION_FILE>(1, stdio_paths.at(1), O_WRONLY, 0), false);
    fs->add_file_entry(2, std::make_shared<ARION_FILE>(2, stdio_paths.at(2), O_WRONLY, 0), false);
    return std::move(fs);
}

std::string FileSystemManager::find_fd_path(int fd)
{
    char link[PATH_MAX];
    std::snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
    if (!std::filesystem::exists(link))
        throw FileNotFoundException(std::string(link));
    ssize_t len = readlink(link, link, sizeof(link) - 1);
    if (len == -1)
        throw ReadLinkFileException(std::string(link));
    link[len] = '\0';
    return std::string(link);
}

FileSystemManager::FileSystemManager(std::weak_ptr<Arion> arion, std::string fs_path, std::string cwd_path)
    : arion(arion)
{
    if (!fs_path.size())
        fs_path = "/";
    if (fs_path.at(fs_path.size() - 1) != '/')
        fs_path += "/";
    if (!cwd_path.size())
        cwd_path = fs_path;
    if (cwd_path.at(cwd_path.size() - 1) != '/')
        cwd_path += "/";
    this->fs_path = fs_path;
    this->cwd_path = cwd_path;
    if (!this->is_in_fs(this->cwd_path))
        throw FileNotInFsException(fs_path, this->cwd_path);
}

std::string FileSystemManager::get_fs_path()
{
    return this->fs_path;
}

std::string FileSystemManager::get_cwd_path()
{
    return this->cwd_path;
}

void FileSystemManager::set_cwd_path(std::string cwd_path)
{
    this->cwd_path = cwd_path;
}

void FileSystemManager::add_file_entry(int target_fd, std::shared_ptr<ARION_FILE> file, bool safe)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto old_file_it = this->files.find(target_fd);
    if (safe)
    {
        if (old_file_it != this->files.end())
            throw FileAlreadyHasFdException(target_fd);
        if (arion->sock->has_socket_entry(target_fd))
            throw SocketAlreadyHasFdException(target_fd);
    }
    this->files[target_fd] = file;
}

bool FileSystemManager::has_file_entry(int target_fd)
{
    return this->files.find(target_fd) != this->files.end();
}

void FileSystemManager::rm_file_entry(int target_fd)
{
    if (this->files.find(target_fd) == this->files.end())
        throw NoFileAtFdException(target_fd);
    this->files.erase(target_fd);
}

std::shared_ptr<ARION_FILE> FileSystemManager::get_arion_file(int target_fd)
{
    if (this->files.find(target_fd) == this->files.end())
        throw NoFileAtFdException(target_fd);
    return this->files.at(target_fd);
}

bool FileSystemManager::can_access_file(std::string path)
{
    std::filesystem::path current = path;
    std::error_code ec;
    while (std::filesystem::is_symlink(current, ec))
    {
        if (access(current.c_str(), R_OK))
            return false;
        std::filesystem::path target = std::filesystem::read_symlink(current, ec);
        if (ec)
            return false;
        if (target.is_relative())
            current = current.parent_path() / target;
        else
            current = target;
    }
    return true;
}

bool FileSystemManager::is_in_fs(std::string path)
{
    if (!this->can_access_file(path))
        return false;
    std::filesystem::path fs_canonical = std::filesystem::weakly_canonical(this->fs_path);
    std::filesystem::path path_canonical = std::filesystem::weakly_canonical(path);
    auto fs_it = fs_canonical.begin();
    auto path_it = path_canonical.begin();
    for (; fs_it != fs_canonical.end(); ++fs_it, ++path_it)
    {
        if (path_it == path_canonical.end() || *path_it != *fs_it)
        {
            return false;
        }
    }
    return true;
}

std::string FileSystemManager::to_fs_path(std::string path)
{
    std::string fmt_path;
    path = strip_str(path);
    if (this->procfs->is_procfs_path(path))
        path = this->procfs->convert(path);
    if (!path.size())
        fmt_path = this->fs_path;
    else if (path.at(0) == '/')
    {

        if (this->is_in_fs(path))
            fmt_path = path;
        else
            fmt_path = this->fs_path + path.substr(1);
    }
    else
        fmt_path = this->cwd_path + path;
    if (!this->is_in_fs(fmt_path))
        fmt_path = "";
    return fmt_path;
}
