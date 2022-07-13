#pragma once

#include "tcp_client.hpp"

#include <memory>
#include <optional>

#include <netinet/in.h>

namespace echo_reverse_server::net {

constexpr uint16_t PORT = 9000;
constexpr int MAX_CONNECTION_NUMBER = 1024;

struct Connection {
    utils::FileDescriptorHolder fd;
    sockaddr_in address;

    uint32_t AddressSize();
    sockaddr* Sockaddr();
};

using ConnectionPtr = std::shared_ptr<Connection>;

class TcpServer {
public:
    TcpServer(uint16_t port, int max_connection_number, bool is_non_blocking = true);
    void Start();

    std::optional<TcpClient> Accept();

    ConnectionPtr Connection();

private:
    int max_connection_number;
    ConnectionPtr connection;
};

} // namespace echo_reverse_server::net
