#pragma once

#include "tcp_client.hpp"

// #include <fmt/format.h>

#include <cerrno>
#include <memory>
#include <optional>
#include <stdexcept>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace echo_reverse_server::net {

struct Connection {
    utils::FileDescriptorHolder fd;
    sockaddr_in address;

    uint32_t AddressSize()
    {
        return sizeof(address);
    }

    sockaddr* Sockaddr()
    {
        return reinterpret_cast<sockaddr*>(&address);
    }
};

using ConnectionPtr = std::shared_ptr<Connection>;

class TcpServer {
public:
    TcpServer(uint16_t port, int max_connection_number, bool is_non_blocking = true)
        : max_connection_number(max_connection_number)
        , connection(std::make_shared<net::Connection>())
    {
        connection->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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

    void Start()
    {
        if (listen(connection->fd, max_connection_number) < 0) {
            throw std::runtime_error("Failed to listen for connections");
        }
    }

    std::optional<TcpClient> Accept()
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

    ConnectionPtr Connection()
    {
        return connection;
    }

private:
    int max_connection_number;
    ConnectionPtr connection;
};

} // namespace echo_reverse_server::net
