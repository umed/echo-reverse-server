
#include "tcp_server.hpp"

#include <cerrno>
#include <stdexcept>

#include <sys/socket.h>
#include <unistd.h>

namespace echo_reverse_server::net {

utils::FileDescriptorHolder fd;
sockaddr_in address;

uint32_t Connection::AddressSize()
{
    return sizeof(address);
}

sockaddr* Connection::Sockaddr()
{
    return reinterpret_cast<sockaddr*>(&address);
}

TcpServer::TcpServer(uint16_t port, int max_connection_number, bool is_non_blocking)
    : max_connection_number(max_connection_number)
    , connection(std::make_shared<net::Connection>())
{
    connection->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!connection->fd.IsValid()) {
        throw std::runtime_error("Failed to create socket");
    }

    auto& address = connection->address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int opt = 1;
    if (setsockopt(connection->fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    if (bind(connection->fd, connection->Sockaddr(), connection->AddressSize()) < 0) {
        throw std::runtime_error("Failed to bind socket to address");
    }

    if (is_non_blocking) {
        utils::SetNonBlocking(connection->fd);
    }
}

// TcpServer(std::string_view ip, uint32_t port, int max_connection_number)
// {
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);
// }

void TcpServer::Start()
{
    if (listen(connection->fd, max_connection_number) < 0) {
        throw std::runtime_error("Failed to listen for connections");
    }
}

std::optional<TcpClient> TcpServer::Accept()
{
    uint32_t addrlen = connection->AddressSize();
    utils::FileDescriptorHolder client_socket = accept(connection->fd, connection->Sockaddr(), &addrlen);
    if (!client_socket.IsValid()) {
        if (utils::WouldBlock()) {
            return std::nullopt;
        }
        throw KernelError("Failed to accept new connection");
    }
    return std::make_optional<TcpClient>(std::move(client_socket));
}

ConnectionPtr TcpServer::Connection()
{
    return connection;
}

} // namespace echo_reverse_server::net
