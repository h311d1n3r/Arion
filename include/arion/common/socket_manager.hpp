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

/// This structure holds data about a UNIX socket.
struct ARION_EXPORT ARION_SOCKET
{
    /// File Descriptor of the socket.
    int fd;
    /// UNIX family of the socket (e.g, AF_UNIX, AF_INET).
    int family;
    /// UNIX type of the socket (e.g, SOCK_STREAM, SOCK_DGRAM).
    int type;
    /// UNIX protocol of the socket.
    int protocol;
    /// IP address associated to the socket, when relevant.
    std::string ip;
    /// Port associated to the socket, when relevant.
    uint16_t port;
    /// Path associated to the socket, when relevant.
    std::string path;
    /// Whether this socket acts as a server.
    bool server = false;
    /// Whether this socket acts as a server and listens to incoming requests.
    bool server_listen = false;
    /// If this socket acts as a server, size of the queue in which other sockets await for connection.
    int server_backlog = 0;
    /// Whether this socket is in blocking mode.
    bool blocking = false;
    /// Length of the socket sockaddr address, when relevant.
    socklen_t s_addr_sz = 0;
    /// The socket sockaddr address, when relevant.
    sockaddr *s_addr = nullptr;
    /**
     * Builder for ARION_SOCKET instances.
     */
    ARION_SOCKET() {};
    /**
     * Builder for ARION_SOCKET instances.
     * @param[in] fd File Descriptor of the socket.
     * @param[in] family UNIX family of the socket (e.g, AF_UNIX, AF_INET).
     * @param[in] type UNIX type of the socket (e.g, SOCK_STREAM, SOCK_DGRAM).
     * @param[in] protocol UNIX protocol of the socket.
     */
    ARION_SOCKET(int fd, int family, int type, int protocol)
        : fd(fd), family(family), type(type), protocol(protocol) {};
    /**
     * Builder for ARION_SOCKET instances used to clone an ARION_SOCKET instance.
     * @param[in] s the ARION_SOCKET instance to be cloned.
     */
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
    /**
     * Builder for ARION_SOCKET instances used to clone an ARION_SOCKET instance.
     * @param[in] s the ARION_SOCKET instance to be cloned.
     */
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
    /**
     * Destructor for ARION_SOCKET instances.
     */
    ~ARION_SOCKET()
    {
        if (s_addr)
            free(s_addr);
        s_addr = nullptr;
    }
};
/*
 * Serializes an ARION_SOCKET instance into a vector of bytes.
 * @param[in] arion_s The ARION_SOCKET to be serialized.
 * @return The serialized vector of bytes.
 */
std::vector<BYTE> serialize_arion_socket(ARION_SOCKET *arion_s);
/*
 * Deserializes an ARION_SOCKET instance from a vector of bytes.
 * @param[in] srz_socket The serialized vector of bytes.
 * @return The deserialized ARION_SOCKET.
 */
ARION_SOCKET *deserialize_arion_socket(std::vector<BYTE> srz_socket);

/// This class handles socket-related operations. It maintains a mapping of file descriptors to
/// their respective ARION_SOCKET instances.
class ARION_EXPORT SocketManager
{
  private:
    /// The Arion instance associated to this instance.
    std::weak_ptr<Arion> arion;

  public:
    /// A map identifying a socket given its UNIX file descriptor.
    std::map<int, std::shared_ptr<ARION_SOCKET>> sockets;
    /**
     * Instanciates and initializes new SocketManager objects with some parameters.
     * @param[in] arion The Arion instance used for emulation.
     * @return A new SocketManager instance.
     */
    static std::unique_ptr<SocketManager> initialize(std::weak_ptr<Arion> arion);
    /**
     * Builder for SocketManager instances.
     * @param[in] arion The Arion instance associated to this instance.
     */
    SocketManager(std::weak_ptr<Arion> arion) : arion(arion) {};
    /**
     * Inserts a new socket to the "sockets" vector.
     * @param[in] target_fd The UNIX file descriptor for the new socket, in the emulation context.
     * @param[in] socket The new socket to be inserted.
     */
    void ARION_EXPORT add_socket_entry(int target_fd, std::shared_ptr<ARION_SOCKET> socket);
    /**
     * Checks whether the "sockets" vector contains a given file descriptor.
     * @param[in] target_fd The UNIX socket descriptor.
     * @return True if the "sockets" vector contains the file descriptor.
     */
    bool ARION_EXPORT has_socket_entry(int target_fd);
    /**
     * Removes a file entry from the "sockets" vector.
     * @param[in] target_fd The UNIX file descriptor of the socket to be removed.
     */
    void ARION_EXPORT rm_socket_entry(int target_fd);
    /**
     * Retrieves an ARION_SOCKET instance from a UNIX file descriptor from the emulation context.
     * @param[in] target_fd The UNIX file descriptor.
     * @return The retrieved ARION_SOCKET.
     */
    std::shared_ptr<ARION_SOCKET> ARION_EXPORT get_arion_socket(int target_fd);
};

}; // namespace arion

#endif // ARION_SOCKET_MANAGER_HPP
