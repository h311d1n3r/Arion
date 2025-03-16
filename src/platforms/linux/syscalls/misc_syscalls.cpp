#include <arion/arion.hpp>
#include <arion/platforms/linux/syscalls/misc_syscalls.hpp>
#include <sys/random.h>

using namespace arion;

uint64_t sys_getrandom(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR buf_addr = params.at(0);
    size_t len = params.at(1);
    unsigned int flags = params.at(2);

    char *buf = (char *)malloc(len);
    ssize_t getrandom_ret = getrandom(buf, len, flags);
    if (getrandom_ret == -1)
        getrandom_ret = -errno;
    else
        arion->mem->write(buf_addr, (BYTE *)buf, len);
    free(buf);
    return getrandom_ret;
}
