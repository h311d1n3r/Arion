#ifndef ARION_SOCKET_MANAGER_HPP
#define ARION_SOCKET_MANAGER_HPP

#include <arion/common/global_defs.hpp>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <vector>

namespace arion
{

class Arion;

struct ARION_EXPORT ARION_SOCKET
{
    int fd;
    int family;
    int type;
    int protocol;
    std::string ip;
    uint16_t port;
    std::string path;
    bool server = false;
    bool server_listen = false;
    int server_backlog = 0;
    bool blocking = false;
    socklen_t s_addr_sz = 0;
    sockaddr *s_addr = nullptr;
    ARION_SOCKET() {};
    ARION_SOCKET(int fd, int family, int type, int protocol)
        : fd(fd), family(family), type(type), protocol(protocol) {};
    ARION_SOCKET(std::shared_ptr<ARION_SOCKET> s)
        : fd(s->fd), family(s->family), type(s->type), protocol(s->protocol), ip(s->ip), port(s->port), path(s->path),
          server(s->server), server_listen(s->server_listen), server_backlog(s->server_backlog), blocking(s->blocking),
          s_addr_sz(s->s_addr_sz)
    {
        if (s->s_addr && s->s_addr_sz)
        {
            s_addr = (sockaddr *)malloc(s->s_addr_sz);
            memcpy(s_addr, s->s_addr, s->s_addr_sz);
        }
    };
    ARION_SOCKET(ARION_SOCKET *s)
        : fd(s->fd), family(s->family), type(s->type), protocol(s->protocol), ip(s->ip), port(s->port), path(s->path),
          server(s->server), server_listen(s->server_listen), server_backlog(s->server_backlog), blocking(s->blocking),
          s_addr_sz(s->s_addr_sz)
    {
        if (s->s_addr && s->s_addr_sz)
        {
            s_addr = (sockaddr *)malloc(s->s_addr_sz);
            memcpy(s_addr, s->s_addr, s->s_addr_sz);
        }
    };
    ~ARION_SOCKET()
    {
        if (s_addr)
            free(s_addr);
        s_addr = nullptr;
    }
};
std::vector<BYTE> serialize_arion_socket(ARION_SOCKET *arion_s);
ARION_SOCKET *deserialize_arion_socket(std::vector<BYTE> srz_socket);

class ARION_EXPORT SocketManager
{
  private:
    std::weak_ptr<Arion> arion;

  public:
    std::map<int, std::shared_ptr<ARION_SOCKET>> sockets;
    static std::unique_ptr<SocketManager> initialize(std::weak_ptr<Arion> arion);
    SocketManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    void ARION_EXPORT add_socket_entry(int target_fd, std::shared_ptr<ARION_SOCKET> socket);
    bool ARION_EXPORT has_socket_entry(int target_fd);
    void ARION_EXPORT rm_socket_entry(int target_fd);
    std::shared_ptr<ARION_SOCKET> ARION_EXPORT get_arion_socket(int target_fd);
};

}; // namespace arion

#endif // ARION_SOCKET_MANAGER_HPP
