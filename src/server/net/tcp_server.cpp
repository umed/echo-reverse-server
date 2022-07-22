
#include "tcp_server.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>

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

std::unique_ptr<TcpSocket> TcpServer::Accept() const
{
    SPDLOG_INFO("Accepting connection");
    int client_socket = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
    if (client_socket < 0) {
        if (utils::WouldBlock()) {
            return nullptr;
        }
        SPDLOG_ERROR("Failed to accept new connection: {}", std::strerror(errno));
        return nullptr;
    }
    return std::make_unique<TcpClient>(client_socket);
}

} // namespace echo_reverse_server::net
