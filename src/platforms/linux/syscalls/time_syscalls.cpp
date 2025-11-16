#include <arion/arion.hpp>
#include <arion/platforms/linux/syscalls/time_syscalls.hpp>

using namespace arion;

uint64_t arion::sys_sched_rr_get_interval(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    pid_t pid = params.at(0);
    ADDR tp_addr = params.at(1);

    struct timespec tp;
    uint64_t sys_sched_rr_get_interval_ret = sched_rr_get_interval(pid, &tp);
    if (sys_sched_rr_get_interval_ret == -1)
        sys_sched_rr_get_interval_ret = -errno;
    else
        arion->mem->write(tp_addr, (BYTE *)&tp, sizeof(struct timespec));
    return sys_sched_rr_get_interval_ret;
}

uint64_t arion::sys_time(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR time_addr = params.at(0);

    time_t t;
    time_t time_ret = time(&t);
    if (time_ret == -1)
        time_ret = -errno;
    else if (time_addr)
        arion->mem->write(time_addr, (BYTE *)&t, sizeof(time_t));
    return time_ret;
}

uint64_t arion::sys_clock_gettime(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    clockid_t clock_id = params.at(0);
    ADDR tp_addr = params.at(1);

    struct timespec tp;
    int gettime_ret = clock_gettime(clock_id, &tp);
    if (gettime_ret == -1)
        gettime_ret = -errno;
    else
        arion->mem->write(tp_addr, (BYTE *)&tp, sizeof(struct timespec));
    return gettime_ret;
}

uint64_t arion::sys_clock_getres(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    clockid_t clock_id = params.at(0);
    ADDR res_addr = params.at(1);

    struct timespec *res_ptr = nullptr;
    if (res_addr)
        res_ptr = (struct timespec *)malloc(sizeof(struct timespec));

    int getres_ret = clock_getres(clock_id, res_ptr);
    if (getres_ret == -1)
        getres_ret = -errno;

    if (res_addr)
    {
        arion->mem->write(res_addr, (BYTE *)res_ptr, sizeof(struct timespec));
        free(res_ptr);
    }

    return getres_ret;
}

uint64_t arion::sys_clock_nanosleep(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    clockid_t clock_id = params.at(0);
    int flags = params.at(1);
    ADDR t_addr = params.at(2);
    ADDR remain_addr = params.at(3);

    if (!arion->config || !arion->config->get_field<bool>("enable_sleep_syscalls"))
        return 0;

    struct timespec t;
    std::vector<BYTE> value(sizeof(t));
    value = arion->mem->read(t_addr, sizeof(t));
    if (value.size() != sizeof(t))
    {
        return -EFAULT;
    }
    std::memcpy(&t, value.data(), sizeof(t));

    struct timespec remain;
    int nanosleep_ret = clock_nanosleep(clock_id, flags, &t, &remain);
    if (nanosleep_ret == -1)
        nanosleep_ret = -errno;

    if (remain_addr)
        arion->mem->write(remain_addr, (BYTE *)&remain, sizeof(struct timespec));

    return nanosleep_ret;
}
