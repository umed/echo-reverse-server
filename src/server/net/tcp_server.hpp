#pragma once

#include "tcp_client.hpp"

#include <memory>
#include <optional>

#include <netinet/in.h>

namespace echo_reverse_server::net {

constexpr uint16_t PORT = 9000;
constexpr int MAX_CONNECTION_NUMBER = 1024;

class TcpServer : public TcpSocket {
public:
    TcpServer(uint16_t port, int max_connection_number);

    void Start() const;
    TcpClient* Accept() const;

private:
    int max_connection_number;
};

} // namespace echo_reverse_server::net
