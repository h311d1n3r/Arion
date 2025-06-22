#ifndef ARION_FS_MANAGER_HPP
#define ARION_FS_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Arion;

struct ARION_EXPORT ARION_FILE
{
    int fd;
    std::string path;
    int flags;
    mode_t mode;
    off_t saved_off = 0;
    ARION_FILE() {};
    ARION_FILE(int fd, std::string path, int flags, mode_t mode) : fd(fd), path(path), flags(flags), mode(mode) {};
    ARION_FILE(std::shared_ptr<ARION_FILE> arion_f)
        : fd(arion_f->fd), path(arion_f->path), flags(arion_f->flags), mode(arion_f->mode),
          saved_off(arion_f->saved_off) {};
    ARION_FILE(ARION_FILE *arion_f)
        : fd(arion_f->fd), path(arion_f->path), flags(arion_f->flags), mode(arion_f->mode),
          saved_off(arion_f->saved_off) {};
};
std::vector<arion::BYTE> serialize_arion_file(ARION_FILE *arion_f);
ARION_FILE *deserialize_arion_file(std::vector<arion::BYTE> srz_file);

class ARION_EXPORT FileSystemManager
{
  private:
    std::weak_ptr<Arion> arion;
    std::string fs_path;
    std::string cwd_path;
    bool can_access_file(std::string path);

  public:
    std::map<int, std::shared_ptr<ARION_FILE>> files;
    static std::unique_ptr<FileSystemManager> initialize(std::weak_ptr<Arion> arion, std::string fs_path,
                                                         std::string cwd_path);
    static std::string find_fd_path(int fd);
    FileSystemManager(std::weak_ptr<Arion> arion, std::string fs_path, std::string cwd_path);
    std::string ARION_EXPORT get_fs_path();
    std::string ARION_EXPORT get_cwd_path();
    void ARION_EXPORT set_cwd_path(std::string cwd_path);
    void ARION_EXPORT add_file_entry(int target_fd, std::shared_ptr<ARION_FILE> file, bool safe = true);
    bool ARION_EXPORT has_file_entry(int target_fd);
    void ARION_EXPORT rm_file_entry(int target_fd);
    std::shared_ptr<ARION_FILE> ARION_EXPORT get_arion_file(int target_fd);
    bool ARION_EXPORT is_in_fs(std::string path);
    std::string ARION_EXPORT to_fs_path(std::string path);
};

#endif // ARION_FS_MANAGER_HPP
