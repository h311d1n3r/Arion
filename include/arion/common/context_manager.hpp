#ifndef ARION_CONTEXT_MANAGER_HPP
#define ARION_CONTEXT_MANAGER_HPP

#include <arion/common/file_system_manager.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/common/socket_manager.hpp>
#include <arion/common/threading_manager.hpp>
#include <memory>
#include <string>
#include <vector>

namespace arion
{

class Arion;

/// Magic string for headers of Arion context files.
const char CONTEXT_FILE_MAGIC[] = "ARIONCTX";
/// Version number of Arion context file format.
const float CONTEXT_FILE_VERSION = 1.0;

/// Stores the whole context of an Arion instance that can be saved and restored to resume emulation from a given point.
struct ARION_EXPORT ARION_CONTEXT
{
    /// TID of the currently running thread.
    pid_t running_tid;
    /// List of all threads in the Arion context.
    std::vector<std::unique_ptr<ARION_THREAD>> thread_list;
    /// List of all futexes in the Arion context.
    std::vector<std::unique_ptr<ARION_FUTEX>> futex_list;
    /// List of all mappings in the Arion context.
    std::vector<std::unique_ptr<ARION_MAPPING>> mapping_list;
    /// List of all open files in the Arion context.
    std::vector<std::unique_ptr<ARION_FILE>> file_list;
    /// List of all open socket connections in the Arion context.
    std::vector<std::unique_ptr<ARION_SOCKET>> socket_list;
    /*
     * Builder for ARION_CONTEXT instances.
     */
    ARION_CONTEXT() {};
    /*
     * Builder for ARION_CONTEXT instances.
     * @param[in] running_tid TID of the currently running thread.
     * @param[in] thread_list List of all threads in the Arion context.
     * @param[in] futex_list List of all futexes in the Arion context.
     * @param[in] mapping_list List of all mappings in the Arion context.
     * @param[in] file_list List of all open files in the Arion context.
     * @param[in] socket_list List of all open socket connections in the Arion context.
     */
    ARION_CONTEXT(pid_t running_tid, std::vector<std::unique_ptr<ARION_THREAD>> thread_list,
                  std::vector<std::unique_ptr<ARION_FUTEX>> futex_list,
                  std::vector<std::unique_ptr<ARION_MAPPING>> mapping_list,
                  std::vector<std::unique_ptr<ARION_FILE>> file_list,
                  std::vector<std::unique_ptr<ARION_SOCKET>> socket_list)
        : running_tid(running_tid), thread_list(std::move(thread_list)), futex_list(std::move(futex_list)),
          mapping_list(std::move(mapping_list)), file_list(std::move(file_list)),
          socket_list(std::move(socket_list)) {};
};

/// This class is used to operate over ARION_CONTEXT instances. It is able to save and load contexts with different
/// restauration modes.
class ARION_EXPORT ContextManager
{
  private:
    /// The Arion instance which context is being managed.
    std::weak_ptr<Arion> arion;

  public:
    /**
     * Instanciates and initializes new ContextManager objects with some parameters.
     * @param[in] arion The Arion instance which context is being managed.
     * @return A new ContextManager instance.
     */
    static std::unique_ptr<ContextManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for ContextManager instances.
     * @param[in] Arion The Arion instance which context is being managed.
     */
    ContextManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Saves the current context of the associated Arion instance into an ARION_CONTEXT instance.
     * @return The ARION_CONTEXT instance.
     */
    std::shared_ptr<ARION_CONTEXT> ARION_EXPORT save();
    /**
     * Restores a saved ARION_CONTEXT into the associated Arion instance.
     * @param[in] ctx The ARION_CONTEXT to be restored.
     * @param[in] restore_mappings True if memory allocations should be restored as in the saved context. This parameter
     * does not condition restoring the memory allocations data.
     * @param[in] restore_data True if the memory allocations data should be restored as in the saved context.
     */
    void ARION_EXPORT restore(std::shared_ptr<ARION_CONTEXT> ctx, bool restore_mappings = true,
                              bool restore_data = true);
    /**
     * Restores a saved ARION_CONTEXT into the associated Arion instance, using a history of memory edits for
     * optimization purpose.
     * @param[in] ctx The ARION_CONTEXT to be restored.
     * @param[in] edits The history of memory operations to be reversed.
     */
    void ARION_EXPORT restore(std::shared_ptr<ARION_CONTEXT> ctx, std::vector<std::shared_ptr<ARION_MEM_EDIT>> edits);
    /**
     * Saves the current context of the associated Arion instance into a dedicated file.
     * @param[in] file_path The path of the output context file.
     */
    void ARION_EXPORT save_to_file(std::string file_path);
    /**
     * Restores a saved context from a file into the associated Arion instance.
     */
    void ARION_EXPORT restore_from_file(std::string file_path);
};

}; // namespace arion

#endif // ARION_CONTEXT_MANAGER_HPP
