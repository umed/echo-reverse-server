#pragma once

#include "net/tcp_client.hpp"

#include <map>
#include <shared_mutex>
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

struct SynchrnoziedConsumers {
    std::shared_mutex mutex;
    FdConsumerMap data;
};

bool ShouldCloseConnection(const epoll_event& event);

void HandleClientEvent(
    events::Epoller& epoller, net::TcpServer& server, SynchrnoziedConsumers& consumers, const epoll_event& event);

void HandleDisconnect(events::Epoller& epoller,
    net::TcpServer& server,
    SynchrnoziedConsumers& consumers,
    const epoll_event& event) noexcept;

void HandleConnect(
    events::Epoller& epoller, net::TcpServer& server, SynchrnoziedConsumers& consumers, const epoll_event& event);

} // namespace echo_reverse_server::event_handlers
