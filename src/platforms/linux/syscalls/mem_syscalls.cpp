#include <arion/arion.hpp>
#include <arion/common/file_system_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/elf_loader.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <arion/platforms/linux/syscalls/mem_syscalls.hpp>
#include <cerrno>
#include <memory>
#include <sys/mman.h>

using namespace arion;
using namespace arion_lnx_type;

ADDR do_mmap(std::shared_ptr<Arion> arion, ADDR addr, size_t len, int prot, int flags, int fd, off_t off)
{
    addr = arion->mem->align_up(addr);
    if (addr < MMAP_MIN_ADDR)
        addr = MMAP_MIN_ADDR;
    len = arion->mem->align_up(len);
    PROT_FLAGS arion_prot = kernel_prot_to_arion_prot(prot);
    ADDR map_addr;
    std::string mapping_name = "[mmap]";
    std::vector<BYTE> data(len);

    if (flags & MAP_ANONYMOUS)
    {
        std::fill(data.begin(), data.end(), 0);
        if (flags & MAP_STACK)
            mapping_name = "[thread_stack]";
    }
    else
    {
        if (!arion->fs->has_file_entry(fd))
            return EACCES;
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        mapping_name = arion_f->path;
        off_t current_pos = lseek(arion_f->fd, 0, SEEK_CUR);
        lseek(arion_f->fd, off, SEEK_SET);
        ssize_t read_sz = read(arion_f->fd, data.data(), len);
        if (read_sz < 0)
            return EBADF;
        lseek(arion_f->fd, current_pos, SEEK_SET);
    }

    if (flags & MAP_FIXED)
    {
        if (!arion->mem->can_map(addr, len))
            arion->mem->unmap(addr, addr + len);
        map_addr = arion->mem->map(addr, len, arion_prot, mapping_name);
    }
    else if (flags & MAP_FIXED_NOREPLACE)
    {
        if (!arion->mem->can_map(addr, len))
            return EEXIST;
        map_addr = arion->mem->map(addr, len, arion_prot, mapping_name);
    }
    else
    {
        if (arion->mem->has_mapping_with_info("[vvar]"))
            addr = arion->mem->get_mapping_by_info("[vvar]")->start_addr;
        else
        {
            switch (arion->arch->get_attrs()->arch_sz)
            {
            case 32: {
                addr = LINUX_32_INTERP_ADDR;
                break;
            }
            case 64: {
                addr = LINUX_64_INTERP_ADDR;
                break;
            }
            default:
                break;
            }
        }
        map_addr = arion->mem->map_anywhere(addr, len, arion_prot, false, mapping_name);
    }

    arion->mem->write(map_addr, data.data(), data.size());
    return map_addr;
}

uint64_t arion::sys_mmap(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR addr = params.at(0);
    size_t len = params.at(1);
    int prot = params.at(2);
    int flags = params.at(3);
    int fd = params.at(4);
    off_t off = params.at(5);

    ADDR map_addr = do_mmap(arion, addr, len, prot, flags, fd, off);

    return map_addr;
}

uint64_t arion::sys_mmap2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR addr = params.at(0);
    size_t len = params.at(1);
    int prot = params.at(2);
    int flags = params.at(3);
    int fd = params.at(4);
    off_t pgoff = params.at(5);

    ADDR map_addr = do_mmap(arion, addr, len, prot, flags, fd, pgoff * ARION_SYSTEM_PAGE_SZ);

    return map_addr;
}

uint64_t arion::sys_mprotect(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR addr = params.at(0);
    size_t len = params.at(1);
    uint8_t kprot = params.at(2);

    PROT_FLAGS arion_prot = kernel_prot_to_arion_prot(kprot);
    arion->mem->protect(addr, addr + len, arion_prot);
    return 0;
}

uint64_t arion::sys_munmap(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR addr = params.at(0);
    size_t len = params.at(1);

    arion->mem->unmap(addr, addr + len);
    return 0;
}

uint64_t arion::sys_brk(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR addr = params.at(0);

    ADDR brk = arion->mem->get_brk();
    if (addr && brk != addr)
    {
        std::shared_ptr<ARION_MAPPING> heap = arion->mem->get_mapping_by_info("[heap]");
        arion->mem->resize_mapping(heap, heap->start_addr, addr);
        arion->mem->set_brk(addr);
        brk = addr;
    }
    return brk;
}
