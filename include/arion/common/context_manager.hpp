#ifndef ARION_CONTEXT_MANAGER_HPP
#define ARION_CONTEXT_MANAGER_HPP

#include <arion/common/file_system_manager.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/common/socket_manager.hpp>
#include <arion/common/threading_manager.hpp>
#include <memory>
#include <string>
#include <vector>

class Arion;

const char CONTEXT_FILE_MAGIC[] = "ARIONCTX";
const float CONTEXT_FILE_VERSION = 1.0;

struct ARION_EXPORT ARION_CONTEXT
{
    pid_t running_tid;
    std::vector<std::unique_ptr<ARION_THREAD>> thread_list;
    std::vector<std::unique_ptr<ARION_FUTEX>> futex_list;
    std::vector<std::unique_ptr<ARION_MAPPING>> mapping_list;
    std::vector<std::unique_ptr<ARION_FILE>> file_list;
    std::vector<std::unique_ptr<ARION_SOCKET>> socket_list;
    ARION_CONTEXT() {};
    ARION_CONTEXT(pid_t running_tid, std::vector<std::unique_ptr<ARION_THREAD>> thread_list,
                  std::vector<std::unique_ptr<ARION_FUTEX>> futex_list,
                  std::vector<std::unique_ptr<ARION_MAPPING>> mapping_list,
                  std::vector<std::unique_ptr<ARION_FILE>> file_list,
                  std::vector<std::unique_ptr<ARION_SOCKET>> socket_list)
        : running_tid(running_tid), thread_list(std::move(thread_list)), futex_list(std::move(futex_list)),
          mapping_list(std::move(mapping_list)), file_list(std::move(file_list)),
          socket_list(std::move(socket_list)) {};
};

class ARION_EXPORT ContextManager
{
  private:
    std::weak_ptr<Arion> arion;

  public:
    static std::unique_ptr<ContextManager> initialize(std::weak_ptr<Arion> arion);
    ContextManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    std::shared_ptr<ARION_CONTEXT> ARION_EXPORT save();
    void ARION_EXPORT restore(std::shared_ptr<ARION_CONTEXT> ctx, bool restore_mappings = true,
                              bool restore_data = true);
    void ARION_EXPORT restore(std::shared_ptr<ARION_CONTEXT> ctx, std::vector<std::shared_ptr<ARION_MEM_EDIT>> edits);
    void ARION_EXPORT save_to_file(std::string file_path);
    void ARION_EXPORT restore_from_file(std::string file_path);
};

#endif // ARION_CONTEXT_MANAGER_HPP
