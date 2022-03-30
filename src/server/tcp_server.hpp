#pragma once

#include "tcp_client.hpp"

// #include <fmt/format.h>

#include <cerrno>
#include <stdexcept>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace echo_reverse_server::net {

class TcpServer {
public:
    TcpServer(uint16_t port, int max_connection_number)
        : max_connection_number(max_connection_number)
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set socket options");
        }
        if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind socket to address");
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
        if (listen(server_fd, max_connection_number) < 0) {
            throw std::runtime_error("Failed to listen for connections");
        }
    }

    TcpClient Accept()
    {
        uint32_t addrlen = sizeof(address);
        utils::FileDescriptorHolder client_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&address), &addrlen);
        if (client_socket < 0) {
            throw std::runtime_error("Failed to accept new connection");
        }
        return TcpClient(std::move(client_socket));
    }

private:
    int max_connection_number;
    utils::FileDescriptorHolder server_fd;
    sockaddr_in address;
};

} // namespace echo_reverse_server::net
