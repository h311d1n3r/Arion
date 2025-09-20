#include <arion/arion.hpp>
#include <arion/common/global_excepts.hpp>
#include <arion/common/socket_manager.hpp>

using namespace arion;

std::vector<BYTE> arion::serialize_arion_socket(ARION_SOCKET *arion_s)
{
    std::vector<BYTE> srz_socket;

    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->fd, (BYTE *)&arion_s->fd + sizeof(int));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->family, (BYTE *)&arion_s->family + sizeof(int));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->type, (BYTE *)&arion_s->type + sizeof(int));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->protocol, (BYTE *)&arion_s->protocol + sizeof(int));
    size_t ip_sz = arion_s->ip.size();
    srz_socket.insert(srz_socket.end(), (BYTE *)&ip_sz, (BYTE *)&ip_sz + sizeof(size_t));
    srz_socket.insert(srz_socket.end(), (BYTE *)arion_s->ip.c_str(), (BYTE *)arion_s->ip.c_str() + ip_sz);
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->port, (BYTE *)&arion_s->port + sizeof(uint16_t));
    size_t path_sz = arion_s->path.size();
    srz_socket.insert(srz_socket.end(), (BYTE *)&path_sz, (BYTE *)&path_sz + sizeof(size_t));
    srz_socket.insert(srz_socket.end(), (BYTE *)arion_s->path.c_str(), (BYTE *)arion_s->path.c_str() + path_sz);
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->server, (BYTE *)&arion_s->server + sizeof(bool));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->server_listen,
                      (BYTE *)&arion_s->server_listen + sizeof(bool));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->server_backlog,
                      (BYTE *)&arion_s->server_backlog + sizeof(int));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->blocking, (BYTE *)&arion_s->blocking + sizeof(bool));
    srz_socket.insert(srz_socket.end(), (BYTE *)&arion_s->s_addr_sz, (BYTE *)&arion_s->s_addr_sz + sizeof(socklen_t));
    if (arion_s->s_addr_sz)
        srz_socket.insert(srz_socket.end(), (BYTE *)arion_s->s_addr, (BYTE *)arion_s->s_addr + arion_s->s_addr_sz);

    return srz_socket;
}

ARION_SOCKET *arion::deserialize_arion_socket(std::vector<BYTE> srz_socket)
{
    ARION_SOCKET *arion_s = new ARION_SOCKET;

    off_t off = 0;
    memcpy(&arion_s->fd, srz_socket.data() + off, sizeof(int));
    off += sizeof(int);
    memcpy(&arion_s->family, srz_socket.data() + off, sizeof(int));
    off += sizeof(int);
    memcpy(&arion_s->type, srz_socket.data() + off, sizeof(int));
    off += sizeof(int);
    memcpy(&arion_s->protocol, srz_socket.data() + off, sizeof(int));
    off += sizeof(int);
    size_t ip_sz;
    memcpy(&ip_sz, srz_socket.data() + off, sizeof(size_t));
    off += sizeof(size_t);
    char *ip = (char *)malloc(ip_sz);
    memcpy(ip, srz_socket.data() + off, ip_sz);
    arion_s->ip = std::string(ip, ip_sz);
    free(ip);
    off += ip_sz;
    memcpy(&arion_s->port, srz_socket.data() + off, sizeof(uint16_t));
    off += sizeof(uint16_t);
    size_t path_sz;
    memcpy(&path_sz, srz_socket.data() + off, sizeof(size_t));
    off += sizeof(size_t);
    char *path = (char *)malloc(path_sz);
    memcpy(path, srz_socket.data() + off, path_sz);
    arion_s->path = std::string(path, path_sz);
    free(path);
    off += path_sz;
    memcpy(&arion_s->server, srz_socket.data() + off, sizeof(bool));
    off += sizeof(bool);
    memcpy(&arion_s->server_listen, srz_socket.data() + off, sizeof(bool));
    off += sizeof(bool);
    memcpy(&arion_s->server_backlog, srz_socket.data() + off, sizeof(int));
    off += sizeof(int);
    memcpy(&arion_s->blocking, srz_socket.data() + off, sizeof(bool));
    off += sizeof(bool);
    memcpy(&arion_s->s_addr_sz, srz_socket.data() + off, sizeof(socklen_t));
    off += sizeof(socklen_t);
    if (arion_s->s_addr_sz)
    {
        arion_s->s_addr = (sockaddr *)malloc(arion_s->s_addr_sz);
        memcpy(arion_s->s_addr, srz_socket.data() + off, arion_s->s_addr_sz);
    }

    return arion_s;
}

std::unique_ptr<SocketManager> SocketManager::initialize(std::weak_ptr<Arion> arion)
{
    return std::move(std::make_unique<SocketManager>(arion));
}

void SocketManager::add_socket_entry(int target_fd, std::shared_ptr<ARION_SOCKET> socket)
{
    std::shared_ptr<Arion> arion = this->arion.lock();
    if (!arion)
        throw ExpiredWeakPtrException("Arion");

    auto old_socket_it = this->sockets.find(target_fd);
    if (old_socket_it != this->sockets.end())
        throw SocketAlreadyHasFdException(target_fd);
    if (arion->fs->has_file_entry(target_fd))
        throw FileAlreadyHasFdException(target_fd);
    this->sockets[target_fd] = socket;
}

bool SocketManager::has_socket_entry(int target_fd)
{
    return this->sockets.find(target_fd) != this->sockets.end();
}

void SocketManager::rm_socket_entry(int target_fd)
{
    if (this->sockets.find(target_fd) == this->sockets.end())
        throw NoSocketAtFdException(target_fd);
    this->sockets.erase(target_fd);
}

std::shared_ptr<ARION_SOCKET> SocketManager::get_arion_socket(int target_fd)
{
    if (this->sockets.find(target_fd) == this->sockets.end())
        throw NoSocketAtFdException(target_fd);
    return this->sockets.at(target_fd);
}
