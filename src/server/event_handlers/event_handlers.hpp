#pragma once

#include "net/tcp_client.hpp"

#include <map>
#include <vector>

struct epoll_event;

namespace echo_reverse_server {

namespace events {

struct Epoller;

}
namespace net {

struct TcpServer;

}

} // namespace echo_reverse_server

namespace echo_reverse_server::event_handlers {

struct Consumer {
    net::TcpClient client;
    std::vector<uint8_t> unsent_data;
};

using FdConsumerMap = std::map<int, Consumer>;

bool ShouldCloseConnection(const epoll_event& event);

void HandleClientEvent(
    events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event);

void HandleDisconnect(
    events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event);

void HandleConnect(
    events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event);

} // namespace echo_reverse_server::event_handlers
