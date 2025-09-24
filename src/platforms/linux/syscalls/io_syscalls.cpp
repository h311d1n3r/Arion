#include "arion/utils/fs_utils.hpp"
#include <arion/arion.hpp>
#include <arion/common/file_system_manager.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/memory_manager.hpp>
#include <arion/platforms/linux/lnx_kernel_utils.hpp>
#include <arion/platforms/linux/lnx_syscall_manager.hpp>
#include <arion/platforms/linux/syscalls/io_syscalls.hpp>
#include <arion/utils/convert_utils.hpp>
#include <asm/termbits.h>
#include <cerrno>
#include <fcntl.h>
#include <linux/net.h>
#include <memory>
#include <netinet/in.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/vfs.h>
#include <sys/xattr.h>

using namespace arion;
using namespace arion_lnx_type;
using namespace arion_poly_struct;
using namespace arion_exception;

std::map<uint8_t, std::pair<std::string, std::function<uint64_t(std::shared_ptr<Arion> arion,
                                                                std::vector<SYS_PARAM> params, bool &cancel)>>>
    socket_sys_to_func = {{SYS_SOCKET, {"socket", sys_socket}},
                          {SYS_BIND, {"bind", sys_bind}},
                          {SYS_CONNECT, {"connect", sys_connect}},
                          {SYS_LISTEN, {"listen", sys_listen}},
                          {SYS_ACCEPT, {"accept", sys_accept}},
                          {SYS_GETSOCKNAME, {"getsockname", sys_getsockname}},
                          {SYS_GETPEERNAME, {"getpeername", sys_getpeername}},
                          {SYS_SOCKETPAIR, {"socketpair", sys_socketpair}},
                          {SYS_SEND, {"send", sys_send}},
                          {SYS_RECV, {"recv", sys_recv}},
                          {SYS_SENDTO, {"sendto", sys_sendto}},
                          {SYS_RECVFROM, {"recvfrom", sys_recvfrom}},
                          {SYS_SHUTDOWN, {"shutdown", sys_shutdown}},
                          {SYS_SETSOCKOPT, {"setsockopt", sys_setsockopt}},
                          {SYS_GETSOCKOPT, {"getsockopt", sys_getsockopt}},
                          {SYS_SENDMSG, {"sendmsg", sys_sendmsg}},
                          {SYS_RECVMSG, {"recvmsg", sys_recvmsg}},
                          {SYS_ACCEPT4, {"accept4", sys_accept4}},
                          {SYS_RECVMMSG, {"recvmmsg", sys_recvmmsg}},
                          {SYS_SENDMMSG, {"sendmmsg", sys_sendmmsg}}};

std::string get_path_at(std::shared_ptr<Arion> arion, int dfd, std::string file_name)
{
    std::string dir_path;
    if (dfd != AT_FDCWD)
    {
        if (!arion->fs->has_file_entry(dfd))
            return std::string();
        std::shared_ptr<ARION_FILE> arion_df = arion->fs->get_arion_file(dfd);
        dir_path = arion_df->path;
    }
    else
    {
        std::string stripped_file_name = strip_str(file_name);
        if (stripped_file_name.size() && stripped_file_name.at(0) == '/')
            dir_path = arion->fs->get_fs_path();
        else
            dir_path = arion->fs->get_cwd_path();
    }
    if (dir_path.size() && file_name.size() && dir_path.at(dir_path.size() - 1) != '/')
        dir_path += std::string("/");
    if (file_name.size() && file_name.at(0) == '/')
        file_name = file_name.substr(1);
    std::string abs_file_path = arion->fs->to_fs_path(dir_path + file_name);
    return abs_file_path;
}

std::string manage_pre_socket_conn(std::shared_ptr<Arion> arion, struct sockaddr *sockaddr_, socklen_t &addr_len)
{
    std::string unix_sock_path;
    if (sockaddr_->sa_family == AF_UNIX)
    {
        struct sockaddr_un *unix_sock = (struct sockaddr_un *)sockaddr_;
        bool is_domain_socket = unix_sock->sun_path[0] == 0;
        unix_sock_path = std::string(unix_sock->sun_path + is_domain_socket, addr_len - 2 - is_domain_socket);
        unix_sock_path = arion->fs->to_fs_path(unix_sock_path);
        size_t unix_sock_path_sz = unix_sock_path.size();
        addr_len = 2 + is_domain_socket + unix_sock_path_sz;
        if (unix_sock_path_sz > ARION_UNIX_PATH_MAX)
            throw PathTooLongException(unix_sock_path);
        memcpy(unix_sock->sun_path + is_domain_socket, unix_sock_path.c_str(), unix_sock_path_sz);
    }
    return unix_sock_path;
}

void manage_post_socket_conn(std::shared_ptr<ARION_SOCKET> arion_s, struct sockaddr *sockaddr_, socklen_t addr_len,
                             std::string unix_sock_path)
{
    if (arion_s->s_addr)
        free(arion_s->s_addr);
    arion_s->s_addr = (sockaddr *)malloc(addr_len);
    arion_s->s_addr_sz = addr_len;
    memcpy(arion_s->s_addr, sockaddr_, addr_len);
    switch (sockaddr_->sa_family)
    {
    case AF_UNIX:
        arion_s->path = unix_sock_path;
        break;
    case AF_INET: {
        struct sockaddr_in *ipv4_sock = (struct sockaddr_in *)sockaddr_;
        arion_s->ip = ipv4_addr_to_string(ipv4_sock->sin_addr.s_addr);
        arion_s->port = htons(ipv4_sock->sin_port);
        break;
    }
    case AF_INET6: {
        struct sockaddr_in6 *ipv6_sock = (struct sockaddr_in6 *)sockaddr_;
        arion_s->ip = ipv6_addr_to_string(ipv6_sock->sin6_addr.s6_addr);
        arion_s->port = htons(ipv6_sock->sin6_port);
        break;
    }
    }
}

uint64_t arion::sys_read(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    uint32_t fd = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t count = params.at(2);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    void *buf = malloc(count);
    int arion_fd;
    bool blocking;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
        blocking = arion_f->blocking;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
        blocking = arion_s->blocking;
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_fd, revents) || !((revents & POLLIN) | (revents & POLLPRI)))
        {
            cancel = true;
            return 0;
        }
    }

    ssize_t read_ret = read(arion_fd, buf, count);
    if (read_ret == -1)
        read_ret = -errno;
    else
        arion->mem->write(buf_addr, (BYTE *)buf, read_ret);
    free(buf);
    return read_ret;
}

uint64_t arion::sys_write(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    uint32_t fd = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t count = params.at(2);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    std::vector<BYTE> buf = arion->mem->read(buf_addr, count);
    int arion_fd;
    bool blocking;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
        blocking = arion_f->blocking;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
        blocking = arion_s->blocking;
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_fd, revents) || !(revents & POLLOUT))
        {
            cancel = true;
            return 0;
        }
    }

    ssize_t write_ret = write(arion_fd, buf.data(), buf.size());
    if (write_ret == -1)
        write_ret = -errno;
    return write_ret;
}

uint64_t arion::sys_open(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    int32_t flags = params.at(1);
    uint16_t mode = params.at(2);

    if (arion->arch->get_attrs()->arch == CPU_ARCH::ARM_ARCH)
        flags &= ~0x20000;

    std::string fs_path = arion->fs->get_fs_path();
    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    int fd = open(file_name_fs.c_str(), flags, mode);
    if (fd == -1)
        fd = -errno;
    else
    {
        std::shared_ptr<ARION_FILE> arion_f = std::make_shared<ARION_FILE>(fd, file_name_fs, flags, mode);
        arion->fs->add_file_entry(fd, arion_f);
    }
    return fd;
}

uint64_t arion::sys_close(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    uint32_t fd = params.at(0);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;

    int close_ret;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        if (arion_f->fd < 3)
            return 0; // Prevent closing standard I/O streams
        close_ret = close(arion_f->fd);
        if (close_ret == -1)
            close_ret = -errno;
        else if (!close_ret)
            arion->fs->rm_file_entry(fd);
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        close_ret = close(arion_s->fd);
        if (close_ret == -1)
            close_ret = -errno;
        else if (!close_ret)
            arion->sock->rm_socket_entry(fd);
    }
    return close_ret;
}

uint64_t arion::sys_newstat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    ADDR stat_buf_addr = params.at(1);

    std::string fs_path = arion->fs->get_fs_path();
    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    struct stat stat_buf;
    uint64_t stat_ret = stat(file_name_fs.c_str(), &stat_buf);
    if (stat_ret == -1)
        stat_ret = -errno;
    else
    {
        STRUCT_ID stat_id = STAT_STRUCT_FACTORY->feed_host(&stat_buf);
        size_t arch_stat_len;
        BYTE *arch_stat = STAT_STRUCT_FACTORY->build(stat_id, arion->arch->get_attrs()->arch, arch_stat_len);
        arion->mem->write(stat_buf_addr, arch_stat, arch_stat_len);
        free(arch_stat);
        STAT_STRUCT_FACTORY->release_struct(stat_id);
    }
    return stat_ret;
}

uint64_t arion::sys_newfstat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int32_t fd = params.at(0);
    ADDR stat_buf_addr = params.at(1);

    if (!arion->fs->has_file_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
    struct stat stat_buf;
    uint64_t fstat_ret = fstat(arion_f->fd, &stat_buf);
    if (fstat_ret == -1)
        fstat_ret = -errno;
    else
    {
        STRUCT_ID stat_id = STAT_STRUCT_FACTORY->feed_host(&stat_buf);
        size_t arch_stat_len;
        BYTE *arch_stat = STAT_STRUCT_FACTORY->build(stat_id, arion->arch->get_attrs()->arch, arch_stat_len);
        arion->mem->write(stat_buf_addr, arch_stat, arch_stat_len);
        free(arch_stat);
        STAT_STRUCT_FACTORY->release_struct(stat_id);
    }
    return fstat_ret;
}

uint64_t arion::sys_newlstat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    ADDR stat_buf_addr = params.at(1);

    std::string fs_path = arion->fs->get_fs_path();
    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    struct stat stat_buf;
    uint64_t lstat_ret = lstat(file_name_fs.c_str(), &stat_buf);
    if (lstat_ret == -1)
        lstat_ret = -errno;
    else
    {
        STRUCT_ID stat_id = STAT_STRUCT_FACTORY->feed_host(&stat_buf);
        size_t arch_stat_len;
        BYTE *arch_stat = STAT_STRUCT_FACTORY->build(stat_id, arion->arch->get_attrs()->arch, arch_stat_len);
        arion->mem->write(stat_buf_addr, arch_stat, arch_stat_len);
        free(arch_stat);
        STAT_STRUCT_FACTORY->release_struct(stat_id);
    }
    return lstat_ret;
}

uint64_t arion::sys_poll(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR fds_addr = params.at(0);
    nfds_t nfds = params.at(1);
    int timeout = params.at(2);

    struct pollfd *fds = (struct pollfd *)calloc(sizeof(struct pollfd), nfds);
    for (size_t fd_i = 0; fd_i < nfds; fd_i++)
    {
        std::vector<BYTE> poll_fd = arion->mem->read(fds_addr + sizeof(struct pollfd) * fd_i, sizeof(struct pollfd));
        memcpy(&fds[fd_i], poll_fd.data(), poll_fd.size());
        bool is_file = arion->fs->has_file_entry(fds[fd_i].fd);
        if (!is_file && !arion->sock->has_socket_entry(fds[fd_i].fd))
            return EBADF;
        if (is_file)
        {
            std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fds[fd_i].fd);
            fds[fd_i].fd = arion_f->fd;
        }
        else
        {
            std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fds[fd_i].fd);
            fds[fd_i].fd = arion_s->fd;
        }
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io)
        timeout = 0;

    int poll_ret = poll(fds, nfds, timeout);
    if (poll_ret == -1)
        poll_ret = -errno;
    else if (poll_ret > 0)
    {
        for (size_t fd_i = 0; fd_i < nfds; fd_i++)
            arion->mem->write_val(fds_addr + offsetof(struct pollfd, revents) + sizeof(struct pollfd) * fd_i,
                                  fds[fd_i].revents, sizeof(short));
    }
    else if (!poll_ret)
        cancel = true;
    return poll_ret;
}

uint64_t arion::sys_lseek(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    unsigned int fd = params.at(0);
    off_t off = params.at(1);
    unsigned int whence = params.at(2);

    if (!arion->fs->has_file_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
    off_t lseek_ret = lseek(arion_f->fd, off, whence);
    if (lseek_ret == -1)
        lseek_ret = -errno;
    return lseek_ret;
}

uint64_t arion::sys_ioctl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    unsigned int fd = params.at(0);
    unsigned int cmd = params.at(1);
    ADDR arg = params.at(2);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    int arion_fd;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
    }
    ADDR saved_arg = arg;
    switch (cmd)
    {
    case TCGETS: {
        struct termios *term = (struct termios *)malloc(sizeof(struct termios));
        arg = (ADDR)term;
        break;
    }
    case TIOCGWINSZ: {
        struct winsize *win_sz = (struct winsize *)malloc(sizeof(struct winsize));
        arg = (ADDR)win_sz;
        break;
    }
    case TIOCGPGRP: {
        return arion->get_pgid();
    }
    default: {
        colorstream warn_msg;
        warn_msg << LOG_COLOR::ORANGE << "Unsupported IOCTL value: " << LOG_COLOR::MAGENTA
                 << int_to_hex<uint64_t>(cmd) << LOG_COLOR::ORANGE << std::string(".");
        arion->logger->warn(warn_msg.str());
        return 0;
    }
    }

    int ioctl_ret = syscall(SYS_ioctl, arion_fd, cmd, arg);

    switch (cmd)
    {
    case TCGETS: {
        struct termios *term = (struct termios *)arg;
        if (ioctl_ret >= 0)
            arion->mem->write(saved_arg, (BYTE *)term, sizeof(struct termios));
        free(term);
        break;
    }
    case TIOCGWINSZ: {
        struct winsize *win_sz = (struct winsize *)arg;
        if (ioctl_ret >= 0)
            arion->mem->write(saved_arg, (BYTE *)win_sz, sizeof(struct winsize));
        free(win_sz);
        break;
    }
    default:
        arion->logger->warn(std::string("Unsupported IOCTL value: ") + int_to_hex<unsigned long>(cmd) +
                            std::string("."));
        return 0;
    }
    return ioctl_ret;
}

uint64_t arion::sys_pread64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t count = params.at(2);
    off_t off = params.at(3);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    void *buf = malloc(count);
    int arion_fd;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
    }
    ssize_t pread_ret = pread64(arion_fd, buf, count, off);
    if (pread_ret == -1)
        pread_ret = -errno;
    else
        arion->mem->write(buf_addr, (BYTE *)buf, count);
    free(buf);
    return pread_ret;
}

uint64_t arion::sys_pwrite64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t count = params.at(2);
    off_t off = params.at(3);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    int arion_fd;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
    }
    std::vector<BYTE> buf = arion->mem->read(buf_addr, count);
    ssize_t pwrite_ret = pwrite64(arion_fd, buf.data(), count, off);
    if (pwrite_ret == -1)
        pwrite_ret = -errno;
    return pwrite_ret;
}

uint64_t arion::sys_readv(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR iov_arr_addr = params.at(1);
    size_t iov_cnt = params.at(2);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    int arion_fd;
    bool blocking = false;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
        blocking = arion_f->blocking;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
        blocking = arion_s->blocking;
    }
    struct iovec *iov = (struct iovec *)calloc(iov_cnt, sizeof(struct iovec));
    uint16_t arch_sz = arion->arch->get_attrs()->arch_sz;
    for (size_t iov_i = 0; iov_i < iov_cnt; iov_i++)
    {
        switch (arch_sz)
        {
        case 64: {
            iov[iov_i].iov_len =
                arion->mem->read_sz(iov_arr_addr + offsetof(struct iovec, iov_len) + sizeof(struct iovec) * iov_i);
            break;
        }
        case 32: {
            iov[iov_i].iov_len =
                arion->mem->read_sz(iov_arr_addr + offsetof(struct iovec32, iov_len) + sizeof(struct iovec32) * iov_i);
            break;
        }
        default:
            break;
        }
        iov[iov_i].iov_base = malloc(iov[iov_i].iov_len);
    }
    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_fd, revents) || !((revents & POLLIN) | (revents & POLLPRI)))
        {
            cancel = true;
            return 0;
        }
    }
    ssize_t readv_ret = readv(arion_fd, iov, iov_cnt);
    if (readv_ret == -1)
        readv_ret = -errno;
    for (size_t iov_i = 0; iov_i < iov_cnt; iov_i++)
    {
        if (readv_ret > 0)
        {
            switch (arch_sz)
            {
            case 64: {
                ADDR iov_base = arion->mem->read_ptr(iov_arr_addr + sizeof(struct iovec) * iov_i);
                arion->mem->write(iov_base, (BYTE *)iov[iov_i].iov_base, iov[iov_i].iov_len);
                break;
            }
            case 32: {
                ADDR iov_base = arion->mem->read_ptr(iov_arr_addr + sizeof(struct iovec32) * iov_i);
                arion->mem->write(iov_base, (BYTE *)iov[iov_i].iov_base, iov[iov_i].iov_len);
                break;
            }
            default:
                break;
            }
        }
        free(iov->iov_base);
    }
    free(iov);
    return readv_ret;
}

uint64_t arion::sys_writev(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR iov_arr_addr = params.at(1);
    size_t iov_cnt = params.at(2);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    int arion_fd;
    bool blocking;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        arion_fd = arion_f->fd;
        blocking = arion_f->blocking;
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        arion_fd = arion_s->fd;
        blocking = arion_s->blocking;
    }

    struct iovec *iov = (struct iovec *)calloc(iov_cnt, sizeof(struct iovec));
    switch (arion->arch->get_attrs()->arch_sz)
    {
    case 64: {
        for (size_t iov_i = 0; iov_i < iov_cnt; iov_i++)
        {
            ADDR iov_base = arion->mem->read_ptr(iov_arr_addr + sizeof(struct iovec) * iov_i);
            iov[iov_i].iov_len =
                arion->mem->read_sz(iov_arr_addr + offsetof(struct iovec, iov_len) + sizeof(struct iovec) * iov_i);
            iov[iov_i].iov_base = malloc(iov[iov_i].iov_len);
            std::vector<BYTE> iov_data = arion->mem->read(iov_base, iov[iov_i].iov_len);
            memcpy(iov[iov_i].iov_base, iov_data.data(), iov_data.size());
        }
        break;
    }
    case 32: {
        for (size_t iov_i = 0; iov_i < iov_cnt; iov_i++)
        {
            ADDR iov_base = arion->mem->read_ptr(iov_arr_addr + sizeof(struct iovec32) * iov_i);
            iov[iov_i].iov_len =
                arion->mem->read_sz(iov_arr_addr + offsetof(struct iovec32, iov_len) + sizeof(struct iovec32) * iov_i);
            iov[iov_i].iov_base = malloc(iov[iov_i].iov_len);
            std::vector<BYTE> iov_data = arion->mem->read(iov_base, iov[iov_i].iov_len);
            memcpy(iov[iov_i].iov_base, iov_data.data(), iov_data.size());
        }
        break;
    }
    default:
        break;
    }
    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_fd, revents) || !(revents & POLLOUT))
        {
            cancel = true;
            return 0;
        }
    }
    ssize_t writev_ret = writev(arion_fd, iov, iov_cnt);
    if (writev_ret == -1)
        writev_ret = -errno;
    for (size_t iov_i = 0; iov_i < iov_cnt; iov_i++)
        free(iov[iov_i].iov_base);
    free(iov);
    return writev_ret;
}

uint64_t arion::sys_access(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    uint16_t mode = params.at(1);

    std::string fs_path = arion->fs->get_fs_path();
    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    uint64_t access_ret = access(file_name_fs.c_str(), mode);
    if (access_ret == -1)
        access_ret = -errno;
    return access_ret;
}

uint64_t arion::sys_select(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int n = params.at(0);
    ADDR inp_addr = params.at(1);
    ADDR outp_addr = params.at(2);
    ADDR exp_addr = params.at(3);
    ADDR tsv_addr = params.at(4);

    fd_set *inp = (fd_set *)calloc(1, sizeof(fd_set));
    fd_set *outp = (fd_set *)calloc(1, sizeof(fd_set));
    fd_set *exp = (fd_set *)calloc(1, sizeof(fd_set));

    if (inp_addr)
    {
        std::vector<BYTE> inp_data = arion->mem->read(inp_addr, sizeof(fd_set));
        memcpy(inp, inp_data.data(), sizeof(fd_set));
    }
    if (outp_addr)
    {
        std::vector<BYTE> outp_data = arion->mem->read(outp_addr, sizeof(fd_set));
        memcpy(outp, outp_data.data(), sizeof(fd_set));
    }
    if (exp_addr)
    {
        std::vector<BYTE> exp_data = arion->mem->read(exp_addr, sizeof(fd_set));
        memcpy(exp, exp_data.data(), sizeof(fd_set));
    }

    struct timeval *tsv = nullptr;
    if (tsv_addr)
    {
        struct timeval timeout;
        std::vector<BYTE> tsv_data = arion->mem->read(tsv_addr, sizeof(struct timeval));
        memcpy(&timeout, tsv_data.data(), sizeof(struct timeval));
        tsv = &timeout;
    }

    int select_ret = select(n, inp, outp, exp, tsv);
    if (select_ret == -1)
        select_ret = -errno;
    else
    {
        if (inp_addr)
            arion->mem->write(inp_addr, (BYTE *)inp, sizeof(fd_set));
        if (outp_addr)
            arion->mem->write(outp_addr, (BYTE *)outp, sizeof(fd_set));
        if (exp_addr)
            arion->mem->write(exp_addr, (BYTE *)exp, sizeof(fd_set));
    }

    free(inp);
    free(outp);
    free(exp);

    return select_ret;
}

uint64_t arion::sys_dup(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int old_fd = params.at(0);

    bool is_file = arion->fs->has_file_entry(old_fd);
    if (!is_file && !arion->sock->has_socket_entry(old_fd))
        return EBADF;
    int ret_fd;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(old_fd);
        ret_fd = dup(arion_f->fd);
        if (ret_fd == -1)
            ret_fd = -errno;
        else
        {
            std::shared_ptr<ARION_FILE> arion_f_cpy = std::make_shared<ARION_FILE>(arion_f);
            arion_f_cpy->fd = ret_fd;
            arion->fs->add_file_entry(ret_fd, arion_f_cpy);
        }
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(old_fd);
        ret_fd = dup(arion_s->fd);
        if (ret_fd == -1)
            ret_fd = -errno;
        else
        {
            std::shared_ptr<ARION_SOCKET> arion_s_cpy = std::make_shared<ARION_SOCKET>(arion_s);
            arion_s_cpy->fd = ret_fd;
            arion->sock->add_socket_entry(ret_fd, arion_s_cpy);
        }
    }
    return ret_fd;
}

uint64_t arion::sys_dup2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int old_fd = params.at(0);
    int new_fd = params.at(1);

    bool is_file = arion->fs->has_file_entry(old_fd);
    if (!is_file && !arion->sock->has_socket_entry(old_fd))
        return EBADF;
    if (arion->fs->has_file_entry(new_fd) || arion->sock->has_socket_entry(new_fd))
        return EBADF;
    int ret_fd;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(old_fd);
        ret_fd = dup(arion_f->fd);
        if (ret_fd == -1)
            ret_fd = -errno;
        else
        {
            std::shared_ptr<ARION_FILE> arion_f_cpy = std::make_shared<ARION_FILE>(arion_f);
            arion_f_cpy->fd = ret_fd;
            arion->fs->add_file_entry(new_fd, arion_f_cpy);
        }
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(old_fd);
        ret_fd = dup(arion_s->fd);
        if (ret_fd == -1)
            ret_fd = -errno;
        else
        {
            std::shared_ptr<ARION_SOCKET> arion_s_cpy = std::make_shared<ARION_SOCKET>(arion_s);
            arion_s_cpy->fd = ret_fd;
            arion->sock->add_socket_entry(new_fd, arion_s_cpy);
        }
    }
    if (ret_fd >= 0)
        return new_fd;
    return ret_fd;
}

uint64_t arion::sys_socket(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int family = params.at(0);
    int type = params.at(1);
    int protocol = params.at(2);

    int fd = socket(family, type, protocol);
    if (fd == -1)
        fd = -errno;
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = std::make_shared<ARION_SOCKET>(fd, family, type, protocol);
        arion->sock->add_socket_entry(fd, arion_s);
    }
    return fd;
}

uint64_t arion::sys_socketcall(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int call = params.at(0);
    ADDR args_addr = params.at(1);

    auto func_it = socket_sys_to_func.find(call);
    if (func_it == socket_sys_to_func.end())
    {
        arion->logger->warn(std::string("No associated function for socket call ") + int_to_hex<uint64_t>(call) +
                            std::string("."));
        return 0;
    }
    auto func_pair = func_it->second;
    std::string func_name = func_pair.first;
    auto func = func_pair.second;
    uint8_t params_n = PARAMS_N_BY_SYSCALL_NAME.at(func_name);
    std::vector<SYS_PARAM> call_params;
    for (uint8_t param_i = 0; param_i < params_n; param_i++)
    {
        unsigned int param = arion->mem->read_val(args_addr + param_i * sizeof(unsigned int), sizeof(unsigned int));
        call_params.push_back(param);
    }
    arion->logger->debug(std::string("socketcall delegates to SYSCALL : ") + func_name);
    return func(arion, call_params, cancel);
}

uint64_t arion::sys_connect(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int sock_fd = params.at(0);
    ADDR sock_addr_addr = params.at(1);
    socklen_t addr_len = params.at(2);

    if (!arion->sock->has_socket_entry(sock_fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(sock_fd);
    std::vector<BYTE> sock_addr = arion->mem->read(sock_addr_addr, addr_len);
    struct sockaddr *sockaddr_ = (struct sockaddr *)malloc(addr_len);
    memcpy(sockaddr_, sock_addr.data(), addr_len);
    std::string unix_sock_path = manage_pre_socket_conn(arion, sockaddr_, addr_len);
    int connect_ret = connect(arion_s->fd, sockaddr_, addr_len);
    if (connect_ret == -1)
        connect_ret = -errno;
    else if (!connect_ret)
        manage_post_socket_conn(arion_s, sockaddr_, addr_len, unix_sock_path);
    free(sockaddr_);
    return connect_ret;
}

uint64_t arion::sys_accept(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR sock_addr_addr = params.at(1);
    ADDR addr_len_addr = params.at(2);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    socklen_t addr_len = 0, addr_len_cpy = 0;
    struct sockaddr *sock_addr = nullptr;
    bool has_addr = sock_addr_addr && addr_len_addr;
    if (has_addr)
    {
        addr_len = arion->mem->read_sz(addr_len_addr);
        if (addr_len)
            sock_addr = (struct sockaddr *)malloc(addr_len);
    }
    addr_len_cpy = addr_len;
    int accept_ret = accept(arion_s->fd, sock_addr, &addr_len);
    if (accept_ret == -1)
        accept_ret = -errno;
    else
    {
        bool is_cut = addr_len_cpy < addr_len;
        if (is_cut)
            addr_len = addr_len_cpy;
        if (has_addr)
        {
            arion->mem->write_sz(addr_len_addr, addr_len);
            arion->mem->write(sock_addr_addr, (BYTE *)sock_addr, is_cut ? addr_len_cpy : addr_len);
        }
        std::shared_ptr<ARION_SOCKET> arion_new_s = std::make_shared<ARION_SOCKET>(arion_s);
        arion_new_s->fd = accept_ret;
        arion->sock->add_socket_entry(accept_ret, arion_new_s);
        if (has_addr && !is_cut)
            manage_post_socket_conn(arion_s, sock_addr, addr_len, arion_s->path);
    }
    if (sock_addr)
        free(sock_addr);
    return accept_ret;
}

uint64_t arion::sys_sendto(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t len = params.at(2);
    int flags = params.at(3);
    ADDR sock_addr_addr = params.at(4);
    socklen_t addr_len = params.at(5);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    std::vector<BYTE> buf = arion->mem->read(buf_addr, len);
    std::string unix_sock_path;
    struct sockaddr *sockaddr_ = nullptr;
    if (sock_addr_addr && addr_len)
    {
        std::vector<BYTE> sock_addr = arion->mem->read(sock_addr_addr, addr_len);
        sockaddr_ = (struct sockaddr *)malloc(sock_addr.size());
        memcpy(sockaddr_, sock_addr.data(), sock_addr.size());
        unix_sock_path = manage_pre_socket_conn(arion, sockaddr_, addr_len);
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && arion_s->blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_s->fd, revents) || !(revents & POLLOUT))
        {
            cancel = true;
            return 0;
        }
    }

    int sendto_ret = sendto(arion_s->fd, buf.data(), len, flags, sockaddr_, addr_len);
    if (sendto_ret == -1)
        sendto_ret = -errno;
    else if (sock_addr_addr && addr_len)
        manage_post_socket_conn(arion_s, sockaddr_, addr_len, unix_sock_path);
    if (sockaddr_)
        free(sockaddr_);
    return sendto_ret;
}

uint64_t arion::sys_send(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    std::vector call_params(params);
    call_params.push_back(0);
    call_params.push_back(0);

    return arion::sys_sendto(arion, call_params, cancel);
}

uint64_t arion::sys_recvfrom(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t len = params.at(2);
    int flags = params.at(3);
    ADDR sock_addr_addr = params.at(4);
    ADDR addr_len_addr = params.at(5);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    BYTE *buf = (BYTE *)malloc(len);
    std::string unix_sock_path;
    struct sockaddr *sockaddr_ = nullptr;
    socklen_t addr_len = 0;
    if (sock_addr_addr && addr_len_addr)
    {
        addr_len = arion->mem->read_sz(addr_len_addr);
        if (addr_len)
        {
            std::vector<BYTE> sock_addr = arion->mem->read(sock_addr_addr, addr_len);
            sockaddr_ = (struct sockaddr *)malloc(sock_addr.size());
            memcpy(sockaddr_, sock_addr.data(), sock_addr.size());
            unix_sock_path = manage_pre_socket_conn(arion, sockaddr_, addr_len);
        }
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && arion_s->blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_s->fd, revents) || !((revents & POLLIN) | (revents & POLLPRI)))
        {
            cancel = true;
            return 0;
        }
    }

    int recvfrom_ret = recvfrom(arion_s->fd, buf, len, flags, sockaddr_, &addr_len);

    if (recvfrom_ret == -1)
        recvfrom_ret = -errno;
    else
    {
        if (sock_addr_addr && addr_len)
        {
            manage_post_socket_conn(arion_s, sockaddr_, addr_len, unix_sock_path);
            arion->mem->write_sz(addr_len_addr, addr_len);
        }
        arion->mem->write(buf_addr, buf, recvfrom_ret);
    }
    if (sockaddr_)
        free(sockaddr_);
    free(buf);
    return recvfrom_ret;
}

uint64_t arion::sys_recv(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    std::vector call_params(params);
    call_params.push_back(0);
    call_params.push_back(0);

    return arion::sys_recvfrom(arion, call_params, cancel);
}

uint64_t arion::sys_sendmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR msg_addr = params.at(1);
    unsigned int flags = params.at(2);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    struct msghdr *msg = (struct msghdr *)malloc(sizeof(struct msghdr));
    memcpy(msg, arion->mem->read(msg_addr, sizeof(struct msghdr)).data(), sizeof(struct msghdr));
    bool has_addr = msg->msg_name && msg->msg_namelen;
    ADDR msg_name_addr;
    if (has_addr)
    {
        msg_name_addr = (ADDR)msg->msg_name;
        msg->msg_name = malloc(msg->msg_namelen);
        memcpy(msg->msg_name, arion->mem->read(msg_name_addr, msg->msg_namelen).data(), msg->msg_namelen);
        manage_pre_socket_conn(arion, (struct sockaddr *)msg->msg_name, msg->msg_namelen);
    }
    bool has_iov = msg->msg_iov && msg->msg_iovlen;
    if (has_iov)
    {
        size_t iov_arr_sz = sizeof(struct iovec) * msg->msg_iovlen;
        ADDR msg_iov_addr = (ADDR)msg->msg_iov;
        msg->msg_iov = (struct iovec *)malloc(iov_arr_sz);
        memcpy(msg->msg_iov, arion->mem->read(msg_iov_addr, iov_arr_sz).data(), iov_arr_sz);
        for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
        {
            struct iovec *iov = &msg->msg_iov[msg_i];
            if (!iov->iov_base || !iov->iov_len)
                continue;
            ADDR iov_base_addr = (ADDR)iov->iov_base;
            iov->iov_base = malloc(iov->iov_len);
            memset(iov->iov_base, 0, iov->iov_len);
            memcpy(iov->iov_base, arion->mem->read(iov_base_addr, iov->iov_len).data(), iov->iov_len);
        }
    }
    bool has_control = msg->msg_control && msg->msg_controllen;
    if (has_control)
    {
        ADDR msg_control_addr = (ADDR)msg->msg_control;
        msg->msg_control = malloc(msg->msg_controllen);
        memcpy(msg->msg_control, arion->mem->read(msg_control_addr, msg->msg_controllen).data(), msg->msg_controllen);
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && arion_s->blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_s->fd, revents) || !(revents & POLLOUT))
        {
            cancel = true;
            return 0;
        }
    }

    ssize_t sendmsg_ret = sendmsg(arion_s->fd, msg, flags);
    if (sendmsg_ret == -1)
        sendmsg_ret = -errno;
    if (has_addr)
        free(msg->msg_name);
    if (has_iov)
    {
        for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
        {
            struct iovec *iov = &msg->msg_iov[msg_i];
            if (!iov->iov_base || !iov->iov_len)
                continue;
            free(iov->iov_base);
        }
        free(msg->msg_iov);
    }
    if (has_control)
        free(msg->msg_control);
    free(msg);
    return sendmsg_ret;
}

uint64_t arion::sys_recvmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR msg_addr = params.at(1);
    unsigned int flags = params.at(2);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    struct msghdr *msg = (struct msghdr *)malloc(sizeof(struct msghdr));
    memcpy(msg, arion->mem->read(msg_addr, sizeof(struct msghdr)).data(), sizeof(struct msghdr));
    bool has_addr = msg->msg_name && msg->msg_namelen;
    ADDR msg_name_addr;
    if (has_addr)
    {
        msg_name_addr = (ADDR)msg->msg_name;
        msg->msg_name = malloc(msg->msg_namelen);
        memcpy(msg->msg_name, arion->mem->read((ADDR)msg_name_addr, msg->msg_namelen).data(), msg->msg_namelen);
        manage_pre_socket_conn(arion, (struct sockaddr *)msg->msg_name, msg->msg_namelen);
    }
    bool has_iov = msg->msg_iov && msg->msg_iovlen;
    std::map<ADDR, ADDR> buf_addresses;
    if (has_iov)
    {
        size_t iov_arr_sz = sizeof(struct iovec) * msg->msg_iovlen;
        ADDR msg_iov_addr = (ADDR)msg->msg_iov;
        msg->msg_iov = (struct iovec *)malloc(iov_arr_sz);
        memcpy(msg->msg_iov, arion->mem->read(msg_iov_addr, iov_arr_sz).data(), iov_arr_sz);
        for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
        {
            struct iovec *iov = &msg->msg_iov[msg_i];
            if (!iov->iov_base || !iov->iov_len)
                continue;
            ADDR iov_base_addr = (ADDR)iov->iov_base;
            iov->iov_base = malloc(iov->iov_len);
            memset(iov->iov_base, 0, iov->iov_len);
            buf_addresses[(ADDR)iov->iov_base] = iov_base_addr;
        }
    }
    bool has_control = msg->msg_control && msg->msg_controllen;
    ADDR msg_control_addr;
    if (has_control)
    {
        msg_control_addr = (ADDR)msg->msg_control;
        msg->msg_control = malloc(msg->msg_controllen);
        memcpy(msg->msg_control, arion->mem->read(msg_control_addr, msg->msg_controllen).data(), msg->msg_controllen);
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && arion_s->blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_s->fd, revents) || !((revents & POLLIN) | (revents & POLLPRI)))
        {
            cancel = true;
            return 0;
        }
    }

    ssize_t recvmsg_ret = recvmsg(arion_s->fd, msg, flags);
    if (recvmsg_ret == -1)
        recvmsg_ret = -errno;
    if (has_addr)
    {
        arion->mem->write(msg_name_addr, (BYTE *)msg->msg_name, msg->msg_namelen);
        free(msg->msg_name);
    }
    if (has_iov)
    {
        for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
        {
            struct iovec *iov = &msg->msg_iov[msg_i];
            if (!iov->iov_base || !iov->iov_len)
                continue;
            if (recvmsg_ret >= 0)
            {
                auto buf_it = buf_addresses.find((ADDR)iov->iov_base);
                arion->mem->write(buf_it->second, (BYTE *)iov->iov_base, iov->iov_len);
            }
            free(iov->iov_base);
        }
        free(msg->msg_iov);
    }
    if (has_control)
    {
        if (recvmsg_ret >= 0)
            arion->mem->write(msg_control_addr, (BYTE *)msg->msg_control, msg->msg_controllen);
        free(msg->msg_control);
    }
    free(msg);
    return recvmsg_ret;
}

uint64_t arion::sys_shutdown(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    int how = params.at(1);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    int shutdown_ret = shutdown(fd, how);
    if (shutdown_ret == -1)
        shutdown_ret = -errno;
    return shutdown_ret;
}

uint64_t arion::sys_bind(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR sock_addr_addr = params.at(1);
    socklen_t addr_len = params.at(2);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    arion_s->server = true;
    struct sockaddr *sock_addr = nullptr;
    std::string unix_sock_path;
    if (sock_addr_addr && addr_len)
    {
        std::vector<BYTE> sock_addr_vec = arion->mem->read(sock_addr_addr, addr_len);
        sock_addr = (struct sockaddr *)malloc(addr_len);
        memcpy(sock_addr, sock_addr_vec.data(), addr_len);
        unix_sock_path = manage_pre_socket_conn(arion, sock_addr, addr_len);
    }
    int bind_ret = bind(arion_s->fd, sock_addr, addr_len);
    if (bind_ret == -1)
        bind_ret = -errno;
    if (sock_addr_addr && addr_len)
    {
        if (!bind_ret)
            manage_post_socket_conn(arion_s, sock_addr, addr_len, unix_sock_path);
        free(sock_addr);
    }
    return bind_ret;
}

uint64_t arion::sys_listen(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    int backlog = params.at(1);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    arion_s->server_listen = true;
    arion_s->server_backlog = backlog;
    int listen_ret = listen(arion_s->fd, backlog);
    if (listen_ret == -1)
        listen_ret = -errno;
    return listen_ret;
}

uint64_t arion::sys_getsockname(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR sock_addr_addr = params.at(1);
    ADDR addr_len_addr = params.at(2);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    struct sockaddr *sock_addr = nullptr;
    socklen_t addr_len = 0, addr_len_cpy = 0;
    if (sock_addr_addr && addr_len_addr)
    {
        addr_len = arion->mem->read_val(addr_len_addr, sizeof(socklen_t));
        if (addr_len > 0)
            sock_addr = (struct sockaddr *)malloc(addr_len);
    }
    addr_len_cpy = addr_len;
    int getsockname_ret = getsockname(arion_s->fd, sock_addr, &addr_len);
    if (getsockname_ret == -1)
        getsockname_ret = -errno;
    else if (!getsockname_ret && addr_len > 0)
    {
        arion->mem->write_val(addr_len_addr, addr_len, sizeof(socklen_t));
        if (sock_addr_addr)
            arion->mem->write(sock_addr_addr, (BYTE *)sock_addr, std::min(addr_len_cpy, addr_len));
    }
    if (sock_addr)
        free(sock_addr);
    return getsockname_ret;
}

uint64_t arion::sys_getpeername(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR sock_addr_addr = params.at(1);
    ADDR addr_len_addr = params.at(2);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    struct sockaddr *sock_addr = nullptr;
    socklen_t addr_len = 0, addr_len_cpy = 0;
    if (sock_addr_addr && addr_len_addr)
    {
        addr_len = arion->mem->read_sz(addr_len_addr);
        if (addr_len > 0)
            sock_addr = (struct sockaddr *)malloc(addr_len);
    }
    addr_len_cpy = addr_len;
    int getpeername_ret = getpeername(arion_s->fd, sock_addr, &addr_len);
    if (getpeername_ret == -1)
        getpeername_ret = -errno;
    else if (!getpeername_ret)
    {
        arion->mem->write_val(addr_len_addr, addr_len, sizeof(socklen_t));
        arion->mem->write(sock_addr_addr, (BYTE *)sock_addr, std::min(addr_len_cpy, addr_len));
    }
    if (sock_addr)
        free(sock_addr);
    return getpeername_ret;
}

uint64_t arion::sys_socketpair(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int family = params.at(0);
    int type = params.at(1);
    int protocol = params.at(2);
    ADDR sock_vec_addr = params.at(3);

    int *sock_vec = nullptr;
    if (sock_vec_addr)
        sock_vec = (int *)malloc(sizeof(int) * 2);
    int socketpair_ret = socketpair(family, type, protocol, sock_vec);
    if (socketpair_ret == -1)
        socketpair_ret = -errno;
    if (sock_vec)
    {
        if (!socketpair_ret)
        {
            std::shared_ptr<ARION_SOCKET> arion_s_1 =
                std::make_shared<ARION_SOCKET>(sock_vec[0], family, type, protocol);
            arion->sock->add_socket_entry(sock_vec[0], arion_s_1);
            std::shared_ptr<ARION_SOCKET> arion_s_2 =
                std::make_shared<ARION_SOCKET>(sock_vec[1], family, type, protocol);
            arion->sock->add_socket_entry(sock_vec[1], arion_s_2);
            arion->mem->write(sock_vec_addr, (BYTE *)sock_vec, sizeof(int) * 2);
        }
        free(sock_vec);
    }
    return socketpair_ret;
}

uint64_t arion::sys_setsockopt(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    int level = params.at(1);
    int opt_name = params.at(2);
    ADDR opt_val_addr = params.at(3);
    socklen_t opt_len = params.at(4);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    std::vector<BYTE> opt_val = arion->mem->read(opt_val_addr, opt_len);
    int setsockopt_ret = setsockopt(arion_s->fd, level, opt_name, opt_val.data(), opt_len);
    if (setsockopt_ret == -1)
        setsockopt_ret = -errno;
    return setsockopt_ret;
}

uint64_t arion::sys_getsockopt(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    int level = params.at(1);
    int opt_name = params.at(2);
    ADDR opt_val_addr = params.at(3);
    ADDR opt_len_addr = params.at(4);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    std::vector<BYTE> opt_len_vec = arion->mem->read(opt_len_addr, sizeof(socklen_t));
    socklen_t opt_len = *((socklen_t *)opt_len_vec.data());
    BYTE *opt_val = (BYTE *)malloc(opt_len);
    int getsockopt_ret = getsockopt(arion_s->fd, level, opt_name, opt_val, &opt_len);
    if (getsockopt_ret == -1)
        getsockopt_ret = -errno;
    else if (!getsockopt_ret)
    {
        arion->mem->write(opt_val_addr, opt_val, opt_len);
        arion->mem->write(opt_len_addr, (BYTE *)&opt_len, sizeof(socklen_t));
    }
    free(opt_val);
    return getsockopt_ret;
}

uint64_t arion::sys_fcntl(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    int cmd = params.at(1);
    uint64_t arg = params.at(2);

    bool is_file = arion->fs->has_file_entry(fd);
    if (!is_file && !arion->sock->has_socket_entry(fd))
        return EBADF;
    int fcntl_ret;
    if (is_file)
    {
        std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
        fcntl_ret = fcntl(arion_f->fd, cmd, arg);
        if (fcntl_ret == -1)
            fcntl_ret = -errno;
        else
        {
            if ((cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC))
            {
                std::shared_ptr<ARION_FILE> arion_f_cpy = std::make_shared<ARION_FILE>(arion_f);
                arion_f_cpy->fd = fcntl_ret;
                arion->fs->add_file_entry(fcntl_ret, arion_f_cpy);
            }
            if (cmd == O_NONBLOCK)
                arion_f->blocking = false;
        }
    }
    else
    {
        std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
        fcntl_ret = fcntl(arion_s->fd, cmd, arg);
        if (fcntl_ret == -1)
            fcntl_ret = -errno;
        else
        {
            if ((cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC))
            {
                std::shared_ptr<ARION_SOCKET> arion_s_cpy = std::make_shared<ARION_SOCKET>(arion_s);
                arion_s_cpy->fd = fcntl_ret;
                arion->sock->add_socket_entry(fcntl_ret, arion_s_cpy);
            }
            if (cmd == O_NONBLOCK)
                arion_s->blocking = false;
        }
    }
    return fcntl_ret;
}

uint64_t arion::sys_truncate(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    long length = params.at(1);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);

    int truncate_ret = truncate(file_name_fs.c_str(), length);
    if (truncate_ret == -1)
        truncate_ret = -errno;

    return truncate_ret;
}

uint64_t arion::sys_ftruncate(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    off_t length = params.at(1);

    if (!arion->fs->has_file_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);

    int ftruncate_ret = ftruncate(arion_f->fd, length);
    if (ftruncate_ret == -1)
        ftruncate_ret = -errno;

    return ftruncate_ret;
}

uint64_t arion::sys_faccessat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int dfd = params.at(0);
    ADDR file_name_addr = params.at(1);
    int mode = params.at(2);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string abs_file_path = get_path_at(arion, dfd, file_name);
    int access_ret = access(abs_file_path.c_str(), mode);

    return access_ret;
}

uint64_t arion::sys_getcwd(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR buf_addr = params.at(0);
    size_t sz = params.at(1);

    std::string cwd = arion->fs->get_cwd_path();
    size_t cwd_sz = cwd.size() + 1;
    if (cwd_sz > sz)
        return ERANGE;
    arion->mem->write_string(buf_addr, cwd);
    return cwd_sz;
}

uint64_t arion::sys_chdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    arion->fs->set_cwd_path(file_name_fs);
    return 0;
}

uint64_t arion::sys_fchdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);

    if (!arion->fs->has_file_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
    arion->fs->set_cwd_path(arion_f->path);
    return 0;
}

uint64_t arion::sys_rename(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR old_name_addr = params.at(0);
    ADDR new_name_addr = params.at(1);

    std::string old_file_name = arion->mem->read_c_string(old_name_addr);
    std::string old_file_name_fs = arion->fs->to_fs_path(old_file_name);
    std::string new_file_name = arion->mem->read_c_string(new_name_addr);
    std::string new_file_name_fs = arion->fs->to_fs_path(new_file_name);
    int rename_ret = rename(old_file_name_fs.c_str(), new_file_name_fs.c_str());
    if (rename_ret == -1)
        rename_ret = -errno;
    return rename_ret;
}

uint64_t arion::sys_mkdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR dir_name_addr = params.at(0);
    mode_t mode = params.at(1);

    std::string dir_name = arion->mem->read_c_string(dir_name_addr);
    std::string dir_name_fs = arion->fs->to_fs_path(dir_name);
    int mkdir_ret = mkdir(dir_name_fs.c_str(), mode);
    if (mkdir_ret == -1)
        mkdir_ret = -errno;
    return mkdir_ret;
}

uint64_t arion::sys_rmdir(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR dir_name_addr = params.at(0);

    std::string dir_name = arion->mem->read_c_string(dir_name_addr);
    std::string dir_name_fs = arion->fs->to_fs_path(dir_name);
    int rmdir_ret = rmdir(dir_name_fs.c_str());
    if (rmdir_ret == -1)
        rmdir_ret = -errno;
    return rmdir_ret;
}

uint64_t arion::sys_creat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    mode_t mode = params.at(1);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    int fd = creat(file_name_fs.c_str(), mode);
    if (fd == -1)
        fd = -errno;
    else
    {
        std::shared_ptr<ARION_FILE> arion_f =
            std::make_shared<ARION_FILE>(fd, file_name_fs, O_WRONLY | O_CREAT | O_TRUNC, mode);
        arion->fs->add_file_entry(fd, arion_f);
    }
    return fd;
}

uint64_t arion::sys_link(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR old_name_addr = params.at(0);
    ADDR new_name_addr = params.at(1);

    std::string old_file_name = arion->mem->read_c_string(old_name_addr);
    std::string old_file_name_fs = arion->fs->to_fs_path(old_file_name);
    std::string new_file_name = arion->mem->read_c_string(new_name_addr);
    std::string new_file_name_fs = arion->fs->to_fs_path(new_file_name);
    int link_ret = link(old_file_name_fs.c_str(), new_file_name_fs.c_str());
    if (link_ret == -1)
        link_ret = -errno;
    return link_ret;
}

uint64_t arion::sys_unlink(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    int unlink_ret = unlink(file_name_fs.c_str());
    if (unlink_ret == -1)
        unlink_ret = -errno;
    return unlink_ret;
}

uint64_t arion::sys_symlink(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR target_addr = params.at(0);
    ADDR link_addr = params.at(1);

    std::string target_file_name = arion->mem->read_c_string(target_addr);
    std::string target_file_name_fs = arion->fs->to_fs_path(target_file_name);
    std::string link_file_name = arion->mem->read_c_string(link_addr);
    std::string link_file_name_fs = arion->fs->to_fs_path(link_file_name);
    int symlink_ret = symlink(target_file_name_fs.c_str(), link_file_name_fs.c_str());
    if (symlink_ret == -1)
        symlink_ret = -errno;
    return symlink_ret;
}

uint64_t arion::sys_readlink(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR path_addr = params.at(0);
    ADDR buf_addr = params.at(1);
    size_t buf_sz = params.at(2);

    std::string file_name = arion->mem->read_c_string(path_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    BYTE *buf = (BYTE *)malloc(buf_sz);
    int readlink_ret;
    if (file_name == "/proc/self/exe" && arion->get_program_args().size())
    {
        std::string resolved_path = std::string(realpath(arion->get_program_args().at(0).c_str(), NULL));
        size_t min_sz = std::min(buf_sz, resolved_path.size());
        memcpy(buf, resolved_path.c_str(), min_sz);
        readlink_ret = min_sz;
    }
    else
        readlink_ret = readlink(file_name_fs.c_str(), (char *)buf, buf_sz);
    if (readlink_ret == -1)
        readlink_ret = -errno;
    else
        arion->mem->write(buf_addr, buf, buf_sz);
    free(buf);
    return readlink_ret;
}

uint64_t arion::sys_statfs(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR file_name_addr = params.at(0);
    ADDR buf_addr = params.at(1);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string file_name_fs = arion->fs->to_fs_path(file_name);
    struct statfs buf;
    uint64_t statfs_ret = statfs(file_name_fs.c_str(), &buf);
    if (statfs_ret == -1)
        statfs_ret = -errno;
    else
        arion->mem->write(buf_addr, (BYTE *)&buf, sizeof(struct statfs));
    return statfs_ret;
}

uint64_t arion::sys_fstatfs(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR buf_addr = params.at(1);

    if (!arion->fs->has_file_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fd);
    struct statfs buf;
    uint64_t fstatfs_ret = fstatfs(arion_f->fd, &buf);
    if (fstatfs_ret == -1)
        fstatfs_ret = -errno;
    else
        arion->mem->write(buf_addr, (BYTE *)&buf, sizeof(struct statfs));
    return fstatfs_ret;
}

uint64_t arion::sys_getdents64(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR dirent_addr = params.at(1);
    unsigned int count = params.at(2);

    struct linux_dirent *dirent = (struct linux_dirent *)malloc(count);
    int getdents64_ret = getdents64(fd, dirent, count);
    if (getdents64_ret == -1)
        getdents64_ret = -errno;
    else
        arion->mem->write(dirent_addr, (BYTE *)dirent, count);
    free(dirent);
    return getdents64_ret;
}

uint64_t arion::sys_openat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int32_t dfd = params.at(0);
    ADDR file_name_addr = params.at(1);
    int32_t flags = params.at(2);
    uint16_t mode = params.at(3);

    if (arion->arch->get_attrs()->arch == CPU_ARCH::ARM_ARCH)
        flags &= ~0x20000;

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string abs_file_path = get_path_at(arion, dfd, file_name);
    int fd = open(abs_file_path.c_str(), flags, mode);
    if (fd == -1)
        fd = -errno;
    else
    {
        std::shared_ptr<ARION_FILE> arion_f = std::make_shared<ARION_FILE>(fd, abs_file_path, flags, mode);
        arion->fs->add_file_entry(fd, arion_f);
    }
    return fd;
}

uint64_t arion::sys_newfstatat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int dfd = params.at(0);
    ADDR file_name_addr = params.at(1);
    ADDR stat_buf_addr = params.at(2);
    int flags = params.at(3);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    struct stat stat_buf;
    std::string abs_file_path = get_path_at(arion, dfd, file_name);
    int fstatat_ret = fstatat(0, abs_file_path.c_str(), &stat_buf, flags);
    if (fstatat_ret == -1)
        fstatat_ret = -errno;
    else
    {
        STRUCT_ID stat_id = STAT_STRUCT_FACTORY->feed_host(&stat_buf);
        size_t arch_stat_len;
        BYTE *arch_stat = STAT_STRUCT_FACTORY->build(stat_id, arion->arch->get_attrs()->arch, arch_stat_len);
        arion->mem->write(stat_buf_addr, arch_stat, arch_stat_len);
        free(arch_stat);
        STAT_STRUCT_FACTORY->release_struct(stat_id);
    }
    return fstatat_ret;
}

uint64_t arion::sys_renameat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int old_dfd = params.at(0);
    ADDR old_name_addr = params.at(1);
    int new_dfd = params.at(2);
    ADDR new_name_addr = params.at(3);

    std::string old_file_name = arion->mem->read_c_string(old_name_addr);
    std::string new_file_name = arion->mem->read_c_string(new_name_addr);
    std::string old_abs_file_name = get_path_at(arion, old_dfd, old_file_name);
    std::string new_abs_file_name = get_path_at(arion, new_dfd, new_file_name);
    if (!old_abs_file_name.size() || !new_abs_file_name.size())
        return EBADF;
    int renameat_ret = rename(old_abs_file_name.c_str(), new_abs_file_name.c_str());
    if (renameat_ret == -1)
        renameat_ret = -errno;
    return renameat_ret;
}

uint64_t arion::sys_readlinkat(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int dfd = params.at(0);
    ADDR file_name_addr = params.at(1);
    ADDR buf_addr = params.at(2);
    size_t buf_sz = params.at(3);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string abs_file_path = get_path_at(arion, dfd, file_name);
    if (!abs_file_path.size())
        return EBADF;
    BYTE *buf = (BYTE *)malloc(buf_sz);
    int readlinkat_ret;
    if (file_name == "/proc/self/exe" && arion->get_program_args().size())
    {
        std::string resolved_path = std::string(realpath(arion->get_program_args().at(0).c_str(), NULL));
        size_t min_sz = std::min(buf_sz, resolved_path.size());
        memcpy(buf, resolved_path.c_str(), min_sz);
        readlinkat_ret = min_sz;
    }
    else
        readlinkat_ret = readlink(abs_file_path.c_str(), (char *)buf, buf_sz);
    if (readlinkat_ret == -1)
        readlinkat_ret = -errno;
    else
        arion->mem->write(buf_addr, buf, buf_sz);
    free(buf);
    return readlinkat_ret;
}

uint64_t arion::sys_pselect6(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int n = params.at(0);
    ADDR inp_addr = params.at(1);
    ADDR outp_addr = params.at(2);
    ADDR exp_addr = params.at(3);
    ADDR tsp_addr = params.at(4);
    ADDR sig_addr = params.at(5);

    fd_set *inp = (fd_set *)calloc(1, sizeof(fd_set));
    fd_set *outp = (fd_set *)calloc(1, sizeof(fd_set));
    fd_set *exp = (fd_set *)calloc(1, sizeof(fd_set));

    if (inp_addr)
    {
        std::vector<BYTE> inp_data = arion->mem->read(inp_addr, sizeof(fd_set));
        memcpy(inp, inp_data.data(), sizeof(fd_set));
    }
    if (outp_addr)
    {
        std::vector<BYTE> outp_data = arion->mem->read(outp_addr, sizeof(fd_set));
        memcpy(outp, outp_data.data(), sizeof(fd_set));
    }
    if (exp_addr)
    {
        std::vector<BYTE> exp_data = arion->mem->read(exp_addr, sizeof(fd_set));
        memcpy(exp, exp_data.data(), sizeof(fd_set));
    }

    struct timespec *tsp = nullptr;
    if (tsp_addr)
    {
        struct timespec timeout;
        std::vector<BYTE> tsp_data = arion->mem->read(tsp_addr, sizeof(struct timespec));
        memcpy(&timeout, tsp_data.data(), sizeof(struct timespec));
        tsp = &timeout;
    }

    sigset_t *sigmask = nullptr;
    if (sig_addr)
    {
        sigset_t sigset;
        std::vector<BYTE> sig_data = arion->mem->read(sig_addr, sizeof(sigset_t));
        memcpy(&sigset, sig_data.data(), sizeof(sigset_t));
        sigmask = &sigset;
    }

    int pselect_ret = pselect(n, inp, outp, exp, tsp, sigmask);
    if (pselect_ret == -1)
        pselect_ret = -errno;
    else
    {
        if (inp_addr)
            arion->mem->write(inp_addr, (BYTE *)inp, sizeof(fd_set));
        if (outp_addr)
            arion->mem->write(outp_addr, (BYTE *)outp, sizeof(fd_set));
        if (exp_addr)
            arion->mem->write(exp_addr, (BYTE *)exp, sizeof(fd_set));
    }

    free(inp);
    free(outp);
    free(exp);

    return pselect_ret;
}

uint64_t arion::sys_ppoll(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR fds_addr = params.at(0);
    nfds_t nfds = params.at(1);
    ADDR timeout_addr = params.at(2);
    ADDR sigmask_addr = params.at(3);
    size_t sigset_sz = params.at(4);

    struct pollfd *fds = (struct pollfd *)calloc(sizeof(struct pollfd), nfds);
    for (size_t fd_i = 0; fd_i < nfds; fd_i++)
    {
        std::vector<BYTE> poll_fd = arion->mem->read(fds_addr + sizeof(struct pollfd) * fd_i, sizeof(struct pollfd));
        memcpy(&fds[fd_i], poll_fd.data(), poll_fd.size());
        bool is_file = arion->fs->has_file_entry(fds[fd_i].fd);
        if (!is_file && !arion->sock->has_socket_entry(fds[fd_i].fd))
            return EBADF;
        if (is_file)
        {
            std::shared_ptr<ARION_FILE> arion_f = arion->fs->get_arion_file(fds[fd_i].fd);
            fds[fd_i].fd = arion_f->fd;
        }
        else
        {
            std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fds[fd_i].fd);
            fds[fd_i].fd = arion_s->fd;
        }
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");

    struct timespec *timeout_ptr = nullptr;
    if (timeout_addr)
    {
        struct timespec timeout;
        std::vector<BYTE> timeout_data = arion->mem->read(timeout_addr, sizeof(struct timespec));
        memcpy(&timeout, timeout_data.data(), sizeof(struct timespec));
        timeout_ptr = &timeout;

        if (!thread_blocking_io)
        {
            timeout_ptr->tv_sec = 0;
            timeout_ptr->tv_nsec = 0;
        }
    }

    sigset_t *sigmask_ptr = nullptr;
    if (sigmask_addr)
    {
        sigset_t sigmask;
        std::vector<BYTE> sigmask_data = arion->mem->read(sigmask_addr, sigset_sz);
        memcpy(&sigmask, sigmask_data.data(), sigset_sz);
        sigmask_ptr = &sigmask;
    }

    int ppoll_ret = ppoll(fds, nfds, timeout_ptr, sigmask_ptr);
    if (ppoll_ret == -1)
        ppoll_ret = -errno;
    else if (ppoll_ret > 0)
    {
        for (size_t fd_i = 0; fd_i < nfds; fd_i++)
        {
            arion->mem->write_val(fds_addr + offsetof(struct pollfd, revents) + sizeof(struct pollfd) * fd_i,
                                  fds[fd_i].revents, sizeof(short));
        }
    }
    else if (!ppoll_ret)
        cancel = true;
    free(fds);
    return ppoll_ret;
}

uint64_t arion::sys_accept4(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR sock_addr_addr = params.at(1);
    ADDR addr_len_addr = params.at(2);
    int flags = params.at(3);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);
    socklen_t addr_len = 0, addr_len_cpy = 0;
    struct sockaddr *sock_addr = nullptr;
    bool has_addr = sock_addr_addr && addr_len_addr;
    if (has_addr)
    {
        addr_len = arion->mem->read_sz(addr_len_addr);
        if (addr_len)
            sock_addr = (struct sockaddr *)malloc(addr_len);
    }
    addr_len_cpy = addr_len;
    int accept_ret = accept4(arion_s->fd, sock_addr, &addr_len, flags);
    if (accept_ret == -1)
        accept_ret = -errno;
    else
    {
        bool is_cut = addr_len_cpy < addr_len;
        if (is_cut)
            addr_len = addr_len_cpy;
        if (has_addr)
        {
            arion->mem->write_sz(addr_len_addr, addr_len);
            arion->mem->write(sock_addr_addr, (BYTE *)sock_addr, is_cut ? addr_len_cpy : addr_len);
        }
        std::shared_ptr<ARION_SOCKET> arion_new_s = std::make_shared<ARION_SOCKET>(arion_s);
        arion_new_s->fd = accept_ret;
        arion->sock->add_socket_entry(accept_ret, arion_new_s);
        if (has_addr && !is_cut)
            manage_post_socket_conn(arion_s, sock_addr, addr_len, arion_s->path);
    }
    if (sock_addr)
        free(sock_addr);
    return accept_ret;
}

uint64_t arion::sys_recvmmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR mmsghdr_addr = params.at(1);
    unsigned int vlen = params.at(2);
    unsigned int flags = params.at(3);
    ADDR timeout_addr = params.at(4);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;

    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);

    struct mmsghdr *msgs = (struct mmsghdr *)malloc(sizeof(struct mmsghdr) * vlen);

    std::vector<BYTE> mmsghdr_data = arion->mem->read(mmsghdr_addr, sizeof(struct mmsghdr) * vlen);
    memcpy(msgs, mmsghdr_data.data(), mmsghdr_data.size());

    struct timespec *timeout = nullptr;
    if (timeout_addr)
    {
        timeout = (struct timespec *)malloc(sizeof(struct timespec));
        std::vector<BYTE> timeout_data = arion->mem->read(timeout_addr, sizeof(struct timespec));
        memcpy(timeout, timeout_data.data(), timeout_data.size());
    }

    std::map<ADDR, ADDR> buf_addresses;

    for (unsigned int i = 0; i < vlen; ++i)
    {
        struct msghdr *msg = &msgs[i].msg_hdr;

        if (msg->msg_name && msg->msg_namelen)
        {
            ADDR msg_name_addr = (ADDR)msg->msg_name;
            msg->msg_name = malloc(msg->msg_namelen);
            std::vector<BYTE> msg_name_data = arion->mem->read(msg_name_addr, msg->msg_namelen);
            memcpy(msg->msg_name, msg_name_data.data(), msg_name_data.size());
            manage_pre_socket_conn(arion, (struct sockaddr *)msg->msg_name, msg->msg_namelen);
            buf_addresses[(ADDR)msg->msg_name] = msg_name_addr;
        }

        if (msg->msg_iov && msg->msg_iovlen)
        {
            size_t iov_arr_sz = sizeof(struct iovec) * msg->msg_iovlen;
            ADDR msg_iov_addr = (ADDR)msg->msg_iov;
            msg->msg_iov = (struct iovec *)malloc(iov_arr_sz);
            std::vector<BYTE> msg_iov_data = arion->mem->read(msg_iov_addr, iov_arr_sz);
            memcpy(msg->msg_iov, msg_iov_data.data(), msg_iov_data.size());

            for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
            {
                struct iovec *iov = &msg->msg_iov[msg_i];
                if (!iov->iov_base || !iov->iov_len)
                    continue;
                ADDR iov_base_addr = (ADDR)iov->iov_base;
                iov->iov_base = malloc(iov->iov_len);
                memset(iov->iov_base, 0, iov->iov_len);
                buf_addresses[(ADDR)iov->iov_base] = iov_base_addr;
            }
        }

        if (msg->msg_control && msg->msg_controllen)
        {
            ADDR msg_control_addr = (ADDR)msg->msg_control;
            msg->msg_control = malloc(msg->msg_controllen);
            std::vector<BYTE> msg_control_data = arion->mem->read(msg_control_addr, msg->msg_controllen);
            memcpy(msg->msg_control, msg_control_data.data(), msg_control_data.size());
        }
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && arion_s->blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_s->fd, revents) || !((revents & POLLIN) | (revents & POLLPRI)))
        {
            cancel = true;
            return 0;
        }
    }

    int recvmmsg_ret = recvmmsg(arion_s->fd, msgs, vlen, flags, timeout);

    if (recvmmsg_ret == -1)
        recvmmsg_ret = -errno;

    for (unsigned int i = 0; i < vlen; ++i)
    {
        struct msghdr *msg = &msgs[i].msg_hdr;

        if (msg->msg_name && msg->msg_namelen)
        {
            if (!recvmmsg_ret)
            {
                ADDR msg_name_addr = buf_addresses.at((ADDR)msgs[i].msg_hdr.msg_name);
                arion->mem->write(msg_name_addr, (BYTE *)msg->msg_name, msg->msg_namelen);
            }
            free(msg->msg_name);
        }

        if (msg->msg_iov && msg->msg_iovlen)
        {
            for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
            {
                struct iovec *iov = &msg->msg_iov[msg_i];
                if (!iov->iov_base || !iov->iov_len)
                    continue;
                if (!recvmmsg_ret)
                {
                    auto buf_it = buf_addresses.find((ADDR)iov->iov_base);
                    arion->mem->write(buf_it->second, (BYTE *)iov->iov_base, iov->iov_len);
                }
                free(iov->iov_base);
            }
            free(msg->msg_iov);
        }

        if (msg->msg_control && msg->msg_controllen)
        {
            if (!recvmmsg_ret)
            {
                ADDR msg_control_addr = (ADDR)msgs[i].msg_hdr.msg_control;
                arion->mem->write(msg_control_addr, (BYTE *)msg->msg_control, msg->msg_controllen);
            }
            free(msg->msg_control);
        }
    }

    if (timeout)
        free(timeout);
    free(msgs);

    return recvmmsg_ret;
}

uint64_t arion::sys_sendmmsg(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int fd = params.at(0);
    ADDR mmsghdr_addr = params.at(1);
    unsigned int vlen = params.at(2);
    unsigned int flags = params.at(3);

    if (!arion->sock->has_socket_entry(fd))
        return EBADF;
    std::shared_ptr<ARION_SOCKET> arion_s = arion->sock->get_arion_socket(fd);

    struct mmsghdr *msgs = (struct mmsghdr *)malloc(sizeof(struct mmsghdr) * vlen);

    std::vector<BYTE> mmsghdr_data = arion->mem->read(mmsghdr_addr, sizeof(struct mmsghdr) * vlen);
    memcpy(msgs, mmsghdr_data.data(), mmsghdr_data.size());

    std::map<ADDR, ADDR> buf_addresses;

    for (unsigned int i = 0; i < vlen; ++i)
    {
        struct msghdr *msg = &msgs[i].msg_hdr;

        if (msg->msg_name && msg->msg_namelen)
        {
            ADDR msg_name_addr = (ADDR)msg->msg_name;
            msg->msg_name = malloc(msg->msg_namelen);
            std::vector<BYTE> msg_name_data = arion->mem->read(msg_name_addr, msg->msg_namelen);
            memcpy(msg->msg_name, msg_name_data.data(), msg_name_data.size());
            buf_addresses[(ADDR)msg->msg_name] = msg_name_addr;
        }

        if (msg->msg_iov && msg->msg_iovlen)
        {
            size_t iov_arr_sz = sizeof(struct iovec) * msg->msg_iovlen;
            ADDR msg_iov_addr = (ADDR)msg->msg_iov;
            msg->msg_iov = (struct iovec *)malloc(iov_arr_sz);
            std::vector<BYTE> msg_iov_data = arion->mem->read(msg_iov_addr, iov_arr_sz);
            memcpy(msg->msg_iov, msg_iov_data.data(), msg_iov_data.size());

            for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
            {
                struct iovec *iov = &msg->msg_iov[msg_i];
                if (!iov->iov_base || !iov->iov_len)
                    continue;
                ADDR iov_base_addr = (ADDR)iov->iov_base;
                iov->iov_base = malloc(iov->iov_len);
                std::vector<BYTE> iov_base_data = arion->mem->read(iov_base_addr, iov->iov_len);
                memcpy(iov->iov_base, iov_base_data.data(), iov_base_data.size());
                buf_addresses[(ADDR)iov->iov_base] = iov_base_addr;
            }
        }

        if (msg->msg_control && msg->msg_controllen)
        {
            ADDR msg_control_addr = (ADDR)msg->msg_control;
            msg->msg_control = malloc(msg->msg_controllen);
            std::vector<BYTE> msg_control_data = arion->mem->read(msg_control_addr, msg->msg_controllen);
            memcpy(msg->msg_control, msg_control_data.data(), msg_control_data.size());
        }
    }

    bool thread_blocking_io = arion->config->get_field<bool>("thread_blocking_io");
    if (!thread_blocking_io && arion_s->blocking)
    {
        short revents = 0;
        if (!check_fd_status(arion_s->fd, revents) || !(revents & POLLOUT))
        {
            cancel = true;
            return 0;
        }
    }

    int sendmmsg_ret = sendmmsg(arion_s->fd, msgs, vlen, flags);

    if (sendmmsg_ret == -1)
        sendmmsg_ret = -errno;

    for (unsigned int i = 0; i < vlen; ++i)
    {
        struct msghdr *msg = &msgs[i].msg_hdr;

        if (msg->msg_name && msg->msg_namelen)
        {
            if (!sendmmsg_ret)
            {
                ADDR msg_name_addr = buf_addresses.at((ADDR)msgs[i].msg_hdr.msg_name);
                arion->mem->write(msg_name_addr, (BYTE *)msg->msg_name, msg->msg_namelen);
            }
            free(msg->msg_name);
        }

        if (msg->msg_iov && msg->msg_iovlen)
        {
            for (size_t msg_i = 0; msg_i < msg->msg_iovlen; msg_i++)
            {
                struct iovec *iov = &msg->msg_iov[msg_i];
                if (!iov->iov_base || !iov->iov_len)
                    continue;
                if (!sendmmsg_ret)
                {
                    auto buf_it = buf_addresses.find((ADDR)iov->iov_base);
                    arion->mem->write(buf_it->second, (BYTE *)iov->iov_base, iov->iov_len);
                }
                free(iov->iov_base);
            }
            free(msg->msg_iov);
        }

        if (msg->msg_control && msg->msg_controllen)
        {
            if (!sendmmsg_ret)
            {
                ADDR msg_control_addr = (ADDR)msgs[i].msg_hdr.msg_control;
                arion->mem->write(msg_control_addr, (BYTE *)msg->msg_control, msg->msg_controllen);
            }
            free(msg->msg_control);
        }
    }

    free(msgs);

    return sendmmsg_ret;
}

uint64_t arion::sys_renameat2(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int old_dfd = params.at(0);
    ADDR old_name_addr = params.at(1);
    int new_dfd = params.at(2);
    ADDR new_name_addr = params.at(3);
    unsigned int flags = params.at(4);

    std::string old_file_name = arion->mem->read_c_string(old_name_addr);
    std::string new_file_name = arion->mem->read_c_string(new_name_addr);
    std::string old_abs_file_name = get_path_at(arion, old_dfd, old_file_name);
    std::string new_abs_file_name = get_path_at(arion, new_dfd, new_file_name);
    if (!old_abs_file_name.size() || !new_abs_file_name.size())
        return EBADF;
    int renameat2_ret = renameat2(0, old_abs_file_name.c_str(), 0, new_abs_file_name.c_str(), flags);
    if (renameat2_ret == -1)
        renameat2_ret = -errno;
    return renameat2_ret;
}

uint64_t arion::sys_statx(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    int dfd = params.at(0);
    ADDR file_name_addr = params.at(1);
    unsigned int flags = params.at(2);
    unsigned int mask = params.at(3);
    ADDR statx_buf_addr = params.at(4);

    std::string file_name = arion->mem->read_c_string(file_name_addr);
    std::string abs_file_path = get_path_at(arion, dfd, file_name);
    if (!abs_file_path.size())
        return EBADF;
    struct statx statx_buf;
    int statx_ret = statx(0, abs_file_path.c_str(), flags, mask, &statx_buf);
    if (statx_ret == -1)
        statx_ret = -errno;
    else
    {
        STRUCT_ID statx_id = STATX_STRUCT_FACTORY->feed_host(&statx_buf);
        size_t arch_statx_len;
        BYTE *arch_statx = STATX_STRUCT_FACTORY->build(statx_id, arion->arch->get_attrs()->arch, arch_statx_len);
        arion->mem->write(statx_buf_addr, arch_statx, arch_statx_len);
        free(arch_statx);
        STATX_STRUCT_FACTORY->release_struct(statx_id);
    }
    return statx_ret;
}

uint64_t arion::sys_getxattr(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR path_addr = params.at(0);
    ADDR name_addr = params.at(1);
    ADDR value_addr = params.at(2);
    size_t size = params.at(3);

    std::string fs_path = arion->fs->get_fs_path();
    std::string file_name = arion->mem->read_c_string(path_addr);
    std::string path = arion->fs->to_fs_path(file_name);
    std::string name = arion->mem->read_c_string(name_addr);

    std::vector<BYTE> value(size);
    ssize_t ret = getxattr(path.c_str(), name.c_str(), value.data(), size);
    if (ret == -1)
    {
        ret = -errno;
    }

    if (ret >= 0)
    {
        arion->mem->write(value_addr, value.data(), ret);
    }

    return ret;
}

uint64_t arion::sys_lgetxattr(std::shared_ptr<Arion> arion, std::vector<SYS_PARAM> params, bool &cancel)
{
    ADDR path_addr = params.at(0);
    ADDR name_addr = params.at(1);
    ADDR value_addr = params.at(2);
    size_t size = params.at(3);

    std::string fs_path = arion->fs->get_fs_path();
    std::string file_name = arion->mem->read_c_string(path_addr);
    std::string path = arion->fs->to_fs_path(file_name);
    std::string name = arion->mem->read_c_string(name_addr);

    std::vector<BYTE> value(size);
    ssize_t ret = lgetxattr(path.c_str(), name.c_str(), value.data(), size);
    if (ret == -1)
    {
        ret = -errno;
    }

    if (ret >= 0)
    {
        arion->mem->write(value_addr, value.data(), ret);
    }

    return ret;
}
