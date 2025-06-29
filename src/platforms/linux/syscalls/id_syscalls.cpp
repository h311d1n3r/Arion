#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/platforms/linux/syscalls/id_syscalls.hpp>

using namespace arion;

uint64_t sys_getpid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    return arion->get_pid();
}

uint64_t sys_getuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    return arion->get_uid();
}

uint64_t sys_getgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    return arion->get_gid();
}

uint64_t sys_setuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    uid_t uid = params.at(0);

    arion->set_uid(uid);
    return 0;
}

uint64_t sys_setgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    gid_t gid = params.at(0);

    arion->set_gid(gid);
    return 0;
}

uint64_t sys_geteuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    return arion->get_euid();
}

uint64_t sys_getegid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    return arion->get_egid();
}

uint64_t sys_setpgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t pid = params.at(0);
    pid_t pgid = params.at(1);

    if (!pid)
        pid = arion->get_pid();
    std::shared_ptr<ArionGroup> group = arion->get_group();
    if (group->has_arion_instance(pid))
    {
        std::shared_ptr<Arion> target = group->get_arion_instance(pid);
        target->set_pgid(pgid);
    }
    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_getppid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    if (arion->has_parent())
        return arion->get_parent()->get_pid();
    return getppid();
}

uint64_t sys_getpgrp(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    return arion->get_pgid();
}

uint64_t sys_setsid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    arion->set_sid(arion->get_pid());
    return 0;
}

uint64_t sys_setreuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    uid_t ruid = params.at(0);
    uid_t euid = params.at(1);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_setregid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    gid_t rgid = params.at(0);
    gid_t egid = params.at(1);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_getgroups(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int group_list_sz = params.at(0);
    ADDR group_list_addr = params.at(1);

    size_t group_sz = group_list_sz * sizeof(gid_t);
    gid_t *group_list = (gid_t *)malloc(group_sz);
    int getgroups_ret = getgroups(group_list_sz, group_list);
    if (getgroups_ret == -1)
        getgroups_ret = -errno;
    else
        arion->mem->write(group_list_addr, (BYTE *)group_list, group_sz);
    return getgroups_ret;
}

uint64_t sys_setgroups(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    int group_list_sz = params.at(0);
    ADDR group_list_addr = params.at(1);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_setresuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    uid_t ruid = params.at(0);
    uid_t euid = params.at(1);
    uid_t suid = params.at(2);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_getresuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR ruid_addr = params.at(0);
    ADDR euid_addr = params.at(1);
    ADDR suid_addr = params.at(2);

    uid_t ruid, euid, suid;
    int getresuid_ret = getresuid(&ruid, &euid, &suid);
    if (getresuid_ret == -1)
        getresuid_ret = -errno;
    else
    {
        arion->mem->write(ruid_addr, (BYTE *)&ruid, sizeof(uid_t));
        arion->mem->write(euid_addr, (BYTE *)&euid, sizeof(uid_t));
        arion->mem->write(suid_addr, (BYTE *)&suid, sizeof(uid_t));
    }
    return getresuid_ret;
}

uint64_t sys_setresgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    gid_t rgid = params.at(0);
    gid_t egid = params.at(1);
    gid_t sgid = params.at(2);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_getresgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    ADDR rgid_addr = params.at(0);
    ADDR egid_addr = params.at(1);
    ADDR sgid_addr = params.at(2);

    gid_t rgid, egid, sgid;
    int getresgid_ret = getresgid(&rgid, &egid, &sgid);
    if (getresgid_ret == -1)
        getresgid_ret = -errno;
    else
    {
        arion->mem->write(rgid_addr, (BYTE *)&rgid, sizeof(gid_t));
        arion->mem->write(egid_addr, (BYTE *)&egid, sizeof(gid_t));
        arion->mem->write(sgid_addr, (BYTE *)&sgid, sizeof(gid_t));
    }
    return getresgid_ret;
}

uint64_t sys_getpgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t pid = params.at(0);

    if (!pid)
        pid = arion->get_pid();
    std::shared_ptr<ArionGroup> group = arion->get_group();
    if (group->has_arion_instance(pid))
    {
        std::shared_ptr<Arion> target = group->get_arion_instance(pid);
        return target->get_pgid();
    }
    int getpgid_ret = getpgid(pid);
    if (getpgid_ret == -1)
        getpgid_ret = -errno;
    return getpgid_ret;
}

uint64_t sys_setfsuid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    uid_t fsuid = params.at(0);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_setfsgid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    gid_t fsgid = params.at(0);

    // PREVENT MODIFICATION OF ARION PROCESS
    return 0;
}

uint64_t sys_getsid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t pid = params.at(0);

    int getsid_ret = getsid(pid);
    if (getsid_ret == -1)
        getsid_ret = -errno;
    return getsid_ret;
}

uint64_t sys_gettid(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params)
{
    pid_t tid = arion->threads->get_running_tid();
    return tid;
}
