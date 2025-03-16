#include <arion/arion.hpp>
#include <arion/common/abi_manager.hpp>
#include <arion/common/global_defs.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <arion/platforms/linux/syscalls/info_syscalls.hpp>
#include <asm/prctl.h>
#include <linux/capability.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

using namespace arion;

uint64_t sys_newuname(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR buf_addr = params.at(0);

    struct utsname buf;
    int uname_ret = uname(&buf);
    if (uname_ret == -1)
        uname_ret = -errno;
    else
        arion->mem->write(buf_addr, (BYTE *)&buf, sizeof(struct utsname));
    return uname_ret;
}

uint64_t sys_gettimeofday(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR tv_addr = params.at(0);
    ADDR tz_addr = params.at(1);

    struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
    if (!tv)
        return EFAULT;

    int gettimeofday_ret = gettimeofday(tv, NULL);

    if (gettimeofday_ret == -1)
        gettimeofday_ret = -errno;
    else
        arion->mem->write(tv_addr, (BYTE *)tv, sizeof(struct timeval));

    return gettimeofday_ret;
}

uint64_t sys_getrlimit(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    unsigned int resource = params.at(0);
    ADDR rlim_addr = params.at(1);

    if (!rlim_addr)
        return EAGAIN;

    struct rlimit *rlim = (struct rlimit *)malloc(sizeof(struct rlimit));
    int getrlimit_ret;
    switch (resource)
    {
    case RLIMIT_STACK: {
        rlim->rlim_cur = 8192 * ARION_SYSTEM_PAGE_SZ;
        rlim->rlim_max = RLIM_INFINITY;
        getrlimit_ret = 0;
        break;
    }
    default: {
        getrlimit_ret = getrlimit(resource, rlim);
        if (getrlimit_ret == -1)
            getrlimit_ret = -errno;
    }
    }
    if (arion->abi->get_attrs()->arch_sz == 32)
    {
        struct rlimit32 *rlim32 = (struct rlimit32 *)malloc(sizeof(struct rlimit32));
        rlim32->rlim_cur = rlim->rlim_cur;
        rlim32->rlim_max = rlim->rlim_max;
        arion->mem->write(rlim_addr, (BYTE *)rlim32, sizeof(struct rlimit32));
        free(rlim32);
    }
    else
        arion->mem->write(rlim_addr, (BYTE *)rlim, sizeof(struct rlimit));
    free(rlim);
    return getrlimit_ret;
}

uint64_t sys_sysinfo(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR info_addr = params.at(0);

    struct sysinfo *info = nullptr;
    if (info_addr)
        info = (struct sysinfo *)malloc(sizeof(struct sysinfo));

    int sysinfo_ret = sysinfo(info);
    if (sysinfo_ret == -1)
        sysinfo_ret = -errno;

    if (info_addr)
        arion->mem->write(info_addr, (BYTE *)info, sizeof(struct sysinfo));

    return sysinfo_ret;
}

uint64_t sys_capget(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR hdrp = params.at(0);
    ADDR data = params.at(1);

    if (!hdrp)
        return EINVAL;
    struct __user_cap_header_struct *header_struct =
        (struct __user_cap_header_struct *)malloc(sizeof(struct __user_cap_header_struct));
    std::vector<BYTE> header_content = arion->mem->read(hdrp, sizeof(struct __user_cap_header_struct));
    memcpy(header_struct, header_content.data(), sizeof(struct __user_cap_header_struct));
    struct __user_cap_data_struct *data_struct = nullptr;
    if (data)
    {
        data_struct = (struct __user_cap_data_struct *)malloc(sizeof(struct __user_cap_data_struct));
        std::vector<BYTE> data_content = arion->mem->read(data, sizeof(struct __user_cap_data_struct));
        memcpy(data_struct, data_content.data(), sizeof(struct __user_cap_data_struct));
    }

    uint64_t capget_ret = syscall(SYS_capget, header_struct, data_struct);

    if (capget_ret == -1)
        capget_ret = -errno;
    else
    {
        arion->mem->write(hdrp, (BYTE *)header_struct, sizeof(struct __user_cap_header_struct));
        if (data)
            arion->mem->write(data, (BYTE *)data_struct, sizeof(struct __user_cap_data_struct));
    }

    return capget_ret;
}

uint64_t sys_capset(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    // PREVENT MODIFICATION OF SYSTEM SETTINGS
    return 0;
}

uint64_t sys_arch_prctl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int op = params.at(0);
    ADDR addr = params.at(1);

    switch (op)
    {
    case ARCH_SET_CPUID:
        // NOT IMPLEMENTED
        break;
    case ARCH_GET_CPUID:
        arion->mem->write_ptr(addr, 1);
        break;
    case ARCH_SET_FS:
        arion->abi->write_reg(UC_X86_REG_FS_BASE, addr);
        break;
    case ARCH_GET_FS:
        arion->mem->write_ptr(addr, arion->abi->read_arch_reg(UC_X86_REG_FS_BASE));
        break;
    case ARCH_SET_GS:
        arion->abi->write_reg(UC_X86_REG_GS_BASE, addr);
        break;
    case ARCH_GET_GS:
        arion->mem->write_ptr(addr, arion->abi->read_arch_reg(UC_X86_REG_GS_BASE));
        break;
    }
    return 0;
}

uint64_t sys_prlimit64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t pid = params.at(0);
    unsigned int resource = params.at(1);
    ADDR new_rlim_addr = params.at(2);
    ADDR old_rlim_addr = params.at(3);

    if (old_rlim_addr)
    {
        struct rlimit *old_rlimit = (struct rlimit *)malloc(sizeof(struct rlimit));
        if (!pid)
        {
            switch (resource)
            {
            case RLIMIT_STACK:
                old_rlimit->rlim_cur = 8192 * ARION_SYSTEM_PAGE_SZ;
                old_rlimit->rlim_max = RLIM64_INFINITY;
            }
        }
        arion->mem->write(old_rlim_addr, (BYTE *)old_rlimit, sizeof(struct rlimit));
        free(old_rlimit);
    }
    return 0;
}

uint64_t sys_getcpu(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR cpu_addr = params.at(0);
    ADDR node_addr = params.at(1);
    ADDR unused_addr = params.at(2);

    if (cpu_addr)
        arion->mem->write_val(cpu_addr, EMPTY, sizeof(unsigned int));
    if (node_addr)
        arion->mem->write_val(node_addr, EMPTY, sizeof(unsigned int));

    return 0;
}
