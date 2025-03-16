#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <sys/mman.h>

using namespace arion;

PROT_FLAGS kernel_prot_to_arion_prot(int kflags)
{
    PROT_FLAGS flags = 0;
    if (kflags & PROT_EXEC)
        flags |= 1;
    if (kflags & PROT_WRITE)
        flags |= 2;
    if (kflags & PROT_READ)
        flags |= 4;
    return flags;
}
