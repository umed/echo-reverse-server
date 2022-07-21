
#include "tcp_server.hpp"

#include <cerrno>
#include <stdexcept>

#include <sys/socket.h>
#include <unistd.h>

namespace echo_reverse_server::net {

TcpServer::TcpServer(uint16_t port, int max_connection_number)
    : TcpSocket(socket(AF_INET, SOCK_STREAM, 0))
    , max_connection_number(max_connection_number)
{
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in address;
    memset(static_cast<void*>(&address), 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket to address");
    }
}

void TcpServer::Start() const
{
    if (listen(fd, max_connection_number) < 0) {
        throw std::runtime_error("Failed to listen for connections");
    }
}

TcpClient* TcpServer::Accept() const
{
    SPDLOG_INFO("Accepting connection");
    sockaddr in_addr;
    socklen_t addrlen = sizeof(in_addr);
    int client_socket = accept(fd, &in_addr, &addrlen);
    if (client_socket <= 0) {
        if (utils::WouldBlock()) {
            return nullptr;
        }
        SPDLOG_ERROR("Failed to accept new connection: {}", std::strerror(errno));
        return nullptr;
    }
    return new TcpClient(client_socket);
}

} // namespace echo_reverse_server::net
