#ifndef ARION_FS_MANAGER_HPP
#define ARION_FS_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace arion
{

class Arion;

/// Stores attributes related to a file which was opened during emulation.
struct ARION_EXPORT ARION_FILE
{
    /// The UNIX file descriptor.
    int fd;
    /// Path to the file.
    std::string path;
    /// Flags which were used to open the file (like in "open" syscall).
    int flags;
    /// Mode which was used to open the file (like in "open" syscall).
    mode_t mode;
    /// True if the file was opened in blocking mode.
    bool blocking = true;
    /// Offset where the cursor is positioned in the file.
    off_t saved_off = 0;
    /*
     * Builder for ARION_FILE instances.
     */
    ARION_FILE() {};
    /*
     * Builder for ARION_FILE instances.
     * @param[in] fd The UNIX file descriptor.
     * @param[in] path Path to the file.
     * @param[in] flags Flags which were used to open the file (like in "open" syscall).
     * @param[in] mode Mode which was used to open the file (like in "open" syscall).
     */
    ARION_FILE(int fd, std::string path, int flags, mode_t mode) : fd(fd), path(path), flags(flags), mode(mode) {};
    /*
     * Builder for ARION_FILE instances. Used to clone an ARION_FILE instance.
     * @param[in] arion_f The ARION_FILE to be cloned.
     */
    ARION_FILE(std::shared_ptr<ARION_FILE> arion_f)
        : fd(arion_f->fd), path(arion_f->path), flags(arion_f->flags), mode(arion_f->mode), blocking(arion_f->blocking),
          saved_off(arion_f->saved_off) {};
    /*
     * Builder for ARION_FILE instances. Used to clone an ARION_FILE instance.
     * @param[in] arion_f The ARION_FILE to be cloned.
     */
    ARION_FILE(ARION_FILE *arion_f)
        : fd(arion_f->fd), path(arion_f->path), flags(arion_f->flags), mode(arion_f->mode), blocking(arion_f->blocking),
          saved_off(arion_f->saved_off) {};
};
/*
 * Serializes an ARION_FILE instance into a vector of bytes.
 * @param[in] arion_f The ARION_FILE to be serialized.
 * @return The serialized vector of bytes.
 */
std::vector<BYTE> serialize_arion_file(ARION_FILE *arion_f);
/*
 * Deserializes an ARION_FILE instance from a vector of bytes.
 * @param[in] srz_file The serialized vector of bytes.
 * @return The deserialized ARION_FILE.
 */
ARION_FILE *deserialize_arion_file(std::vector<BYTE> srz_file);

/// This class is used to emulate the behavior of the procfs filesystem.
class ProcFSManager
{
  private:
    /// This regex is used to check whether a file path is located inside the system procfs.
    static const inline std::regex PROCFS_REGEX = std::regex(R"(^/proc/([0-9]+|self)/([^/]+)$)");
    /// The Arion instance used for emulation.
    std::weak_ptr<Arion> arion;

  public:
    /*
     * Instanciates and initializes new ProcFSManager objects with some parameters.
     * @param[in] arion The Arion instance used for emulation.
     * @return A new ProcFSManager instance.
     */
    static std::unique_ptr<ProcFSManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for ProcFSManager instances.
     * @param[in] rion The Arion instance used for emulation.
     */
    ProcFSManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Checks whether the given path is located inside the system procfs.
     * @param[in] path The path to check.
     * @return True if the given path is located inside the system procfs.
     */
    bool is_procfs_path(std::string path);
    /**
     * Converts a procfs path into a rootfs path.
     * @param[in] path The procfs path.
     * @return The converted rootfs path.
     */
    std::string convert(std::string path);
};

/// This class is used to emulate interactions with the rootfs specified by user.
class ARION_EXPORT FileSystemManager
{
  private:
    /// The Arion instance used for emulation.
    std::weak_ptr<Arion> arion;
    /// Path to the user specified rootfs.
    std::string fs_path;
    /// Path to the user specifed current working directory.
    std::string cwd_path;
    /// Used to emulate the behavior of the procfs filesystem.
    std::unique_ptr<ProcFSManager> procfs;
    /**
     * Checks whether a given path can be accessed by the user or not, based on his rights.
     * @param[in] path The path whose accessibility is to be checked.
     * @return True if the user is allowed to access the given path.
     */
    bool can_access_file(std::string path);

  public:
    /// A map identifying an open file given its UNIX file descriptor.
    std::map<int, std::shared_ptr<ARION_FILE>> files;
    /**
     * Instanciates and initializes new FileSystemManager objects with some parameters.
     * @param[in] arion The Arion instance used for emulation.
     * @param[in] fs_path Path to the user specified rootfs.
     * @param[in] cwd_path Path to the user specifed current working directory.
     * @return A new FileSystemManager instance.
     */
    static std::unique_ptr<FileSystemManager> initialize(std::weak_ptr<Arion> arion, std::string fs_path,
                                                         std::string cwd_path);
    /**
     * Converts a file descriptor from the emulator context (not the emulation one) into its corresponding system path.
     * @param[in] fd The file descriptor to convert.
     * @return The retrieved system path.
     */
    static std::string find_fd_path(int fd);
    /**
     * Builder for FileSystemManager instances.
     * @param[in] arion The Arion instance used for emulation.
     * @param[in] fs_path Path to the user specified rootfs.
     * @param[in] cwd_path Path to the user specifed current working directory.
     */
    FileSystemManager(std::weak_ptr<Arion> arion, std::string fs_path, std::string cwd_path);
    /**
     * Retrieves the user specified path to rootfs.
     * @return Path to the rootfs.
     */
    std::string ARION_EXPORT get_fs_path();
    /**
     * Retrieves the user specified path to the current working directory.
     * @return Path to the current working directory.
     */
    std::string ARION_EXPORT get_cwd_path();
    /**
     * Defines path to the current working directory.
     * @param[in] cwd_path Path to the new current working directory.
     */
    void ARION_EXPORT set_cwd_path(std::string cwd_path);
    /**
     * Inserts a new open file to the "files" vector.
     * @param[in] target_fd The UNIX file descriptor for the new file, in the emulation context.
     * @param[in] file The new file to be inserted.
     * @param[in] safe If true, additional checks are performed to make sure the file can be inserted safely.
     */
    void ARION_EXPORT add_file_entry(int target_fd, std::shared_ptr<ARION_FILE> file, bool safe = true);
    /**
     * Checks whether the "files" vector contains a given file descriptor.
     * @param[in] target_fd The UNIX file descriptor.
     * @return True if the "files" vector contains the file descriptor.
     */
    bool ARION_EXPORT has_file_entry(int target_fd);
    /**
     * Removes a file entry from the "files" vector.
     * @param[in] target_fd The UNIX file descriptor of the file to be removed.
     */
    void ARION_EXPORT rm_file_entry(int target_fd);
    /**
     * Retrieves an ARION_FILE instance from a UNIX file descriptor from the emulation context.
     * @param[in] target_fd The UNIX file descriptor;
     * @return The retrieved ARION_FILE.
     */
    std::shared_ptr<ARION_FILE> ARION_EXPORT get_arion_file(int target_fd);
    /**
     * Checks whether the given path exists and belongs to the user specified rootfs.
     * @param[in] The path to check.
     * @return True if the given path exists and belongs to the user specified rootfs.
     */
    bool ARION_EXPORT is_in_fs(std::string path);
    /**
     * Converts a given path from the emulation context into its absolute representation inside the user specified
     * rootfs.
     * @param[in] path The path to be converted.
     * @returns The converted path which belongs to the rootfs.
     */
    std::string ARION_EXPORT to_fs_path(std::string path);
};

}; // namespace arion

#endif // ARION_FS_MANAGER_HPP
