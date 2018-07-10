#include "common.h"

/**
 * A non-blocking wrapper of bsd socket with hap network compliance
 *
 * Runs only on POSIX systems
 */
#ifdef USE_HAP_NATIVE_SOCKET

#include "network.h"

#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>

#define HAP_BSD_ERRLOG(f) HAP_DEBUG(f "(%d): %s", errno, strerror(errno))
#define N_RET(ee, f, ret) \
    if((ee) < 0){ \
        HAP_BSD_ERRLOG(f); \
        return ret; \
    }

using namespace std;
struct _hap_bsdsock {
    pollfd pfd;
    enum _hap_bsdsock_type { LISTENING_FD, CLIENT_FD } type;
    hap_network_connection * conn;
    sockaddr_in addr;
    int& fd;

    _hap_bsdsock(int _fd, _hap_bsdsock_type type, hap_network_connection * conn, sockaddr_in addr = {}):
            pfd { .fd = _fd, .events = POLLIN, .revents = 0 },
            type(type), conn(conn), fd(pfd.fd), addr(addr) {}
};
static auto _conn_pool = vector<_hap_bsdsock*>();

bool hap_network_init_bind(hap_network_connection * conn, uint16_t port){
    int server_fd;
    sockaddr_in server_addr {
            .sin_family = AF_INET,
            .sin_port = htons(port), // NOLINT
            .sin_addr.s_addr = htonl(INADDR_ANY) // NOLINT
    };

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    N_RET(server_fd, "socket", false);
    auto hap_fdstore = new _hap_bsdsock(
            server_fd,
            _hap_bsdsock::LISTENING_FD,
            conn);

    auto opt_v = 1; //enable reuseaddr
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_v, sizeof(opt_v));

    opt_v = HAP_SOCK_TCP_NODELAY;
    setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, &opt_v, sizeof(opt_v));

    auto eno = bind(server_fd, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
    N_RET(eno, "bind", false);

    eno = listen(server_fd, 64);
    N_RET(eno, "listen", false);

    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    _conn_pool.push_back(hap_fdstore);
    conn->raw = hap_fdstore;
    return true;
}

bool hap_network_send(hap_network_connection * client, const uint8_t * data, unsigned int length){
    if(client->raw == nullptr) return false;
    auto client_fd = static_cast<_hap_bsdsock*>(client->raw);
    auto eno = send(client_fd->fd, data, length, 0);
    N_RET(eno, "send", false);
    return true;
}

void hap_network_close(hap_network_connection * client){
    if(client->raw == nullptr) return;
    hap_event_network_close(client);
    auto client_fd = static_cast<_hap_bsdsock*>(client->raw);
    shutdown(client_fd->fd, 2);

    //delete from pool
    auto it = find(_conn_pool.begin(), _conn_pool.end(), client_fd);
    _conn_pool.erase(it);

    auto addr_buf = new char[64]();
    inet_ntop(client_fd->addr.sin_family, &(client_fd->addr.sin_addr), addr_buf, 64);
    HAP_DEBUG("Client %s:%u closed the connection", addr_buf, client_fd->addr.sin_port);
    delete[] addr_buf;

    delete client_fd;
    client->raw = nullptr;
    delete client;
}

void _hap_bsd_client_accept(_hap_bsdsock * sock){
    auto addr = sockaddr_in();
    socklen_t sin_size = sizeof(addr);

    auto client_fd = accept(sock->fd, reinterpret_cast<sockaddr *>(&addr), &sin_size);
    N_RET(client_fd, "accept", HAP_NOTHING);
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    auto * client_conn = new hap_network_connection;
    auto client_sock = new _hap_bsdsock(
            client_fd,
            _hap_bsdsock::CLIENT_FD,
            client_conn,
            addr);
    client_conn->raw = client_sock;
    _conn_pool.push_back(client_sock);

    hap_event_network_accept(sock->conn, client_conn);

    auto addr_buf = new char[64]();
    inet_ntop(addr.sin_family, &(addr.sin_addr), addr_buf, 64);
    HAP_DEBUG("New connection: %s:%u", addr_buf, addr.sin_port);
    delete[] addr_buf;
}

void _hap_bsd_client_ondata(_hap_bsdsock * sock){
    auto buf = new uint8_t[1024];
    auto bread = recv(sock->fd, buf, 1024, MSG_DONTWAIT);
    N_RET(bread, "recv", HAP_NOTHING);
    if(bread > 0) hap_event_network_receive(sock->conn, buf, static_cast<unsigned int>(bread));
    delete[] buf;
}

void _hap_bsd_client_close(_hap_bsdsock * sock){
    hap_network_close(sock->conn);
}

void hap_network_loop(){
    pollfd pfds[_conn_pool.size()];
    transform(_conn_pool.begin(), _conn_pool.end(), pfds, [](_hap_bsdsock * sock){ return sock->pfd; });

    //New connections ready to accept
    if(poll(pfds, static_cast<nfds_t>(_conn_pool.size()), 0) > 0){
        for(auto i = 0; i < _conn_pool.size(); ++i){
            auto bsdsock = _conn_pool[i];
            //New data available
            if(pfds[i].revents & POLLIN){
                if(bsdsock->type == _hap_bsdsock::LISTENING_FD){ _hap_bsd_client_accept(bsdsock); }
                else if(bsdsock->type == _hap_bsdsock::CLIENT_FD){ _hap_bsd_client_ondata(bsdsock); }
            }

            if(pfds[i].revents & POLLHUP){
                if(bsdsock->type == _hap_bsdsock::CLIENT_FD){ _hap_bsd_client_close(bsdsock); }
            }

            pfds[i].revents = 0;
        }
    }
}

#endif
