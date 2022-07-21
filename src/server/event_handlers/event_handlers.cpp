#include "event_handlers.hpp"

#include "events/epoller.hpp"
#include "net/tcp_client.hpp"
#include "net/tcp_server.hpp"

#include <spdlog/spdlog.h>

#include <vector>

namespace echo_reverse_server::event_handlers {

namespace {

using BufferArray = std::array<uint8_t, 1024>;

void RemoveTcpSocket(net::TcpSocket* tcp_socket)
{
    SPDLOG_INFO("Closing connection for fd: {}", tcp_socket->GetFd());
    delete tcp_socket;
}

void ReverseEcho(const net::TcpClient& client, const std::vector<uint8_t>& unsent_data)
{
    BufferArray reversed_buffer;
    size_t j = 0;
    for (ssize_t i = unsent_data.size() - 1; i > -1;) {
        for (j = 0; j < reversed_buffer.size() - 1 && i > -1; --i, ++j) {
            reversed_buffer[j] = unsent_data[i];
        }
        if (j == reversed_buffer.size()) {
            client.Write(reversed_buffer, j);
        }
    }
    if (j % reversed_buffer.size() != 0) {
        reversed_buffer[j] = 0;
        ++j;
    }
    client.Write(reversed_buffer, j);
}

void HandleStatus(events::Epoller& epoller, epoll_event& event, const net::ClientStatus status)
{
    auto client = static_cast<net::TcpSocket*>(event.data.ptr);
    SPDLOG_INFO("Handling net::ClientStatus");
    switch (status) {
    case net::ClientStatus::Close:
        SPDLOG_INFO("ClientStatus::Close");
        RemoveTcpSocket(client);
        break;
    case net::ClientStatus::Reschedule:
        SPDLOG_INFO("ClientStatus::Reschedule");
        epoller.Rearm(client->GetFd(), event.data.ptr);
        break;
    default:
        SPDLOG_ERROR("Unexpected Read's status");
    }
}

} // namespace

bool ShouldCloseConnection(const epoll_event& event)
{
    return (event.events & EPOLLERR) || (event.events & EPOLLHUP) // || (event.events & EPOLLRDHUP)
        || (!(event.events & EPOLLIN));
}

net::ClientStatus HandleReadEvent(epoll_event& event)
{
    BufferArray buffer;
    net::TcpReadStatus read_status;
    do {
        read_status = client->Read(buffer);
        if (std::holds_alternative<net::ClientStatus>(read_status)) {
            SPDLOG_INFO("Has got net::ClientStatus");
            break;
        }
        auto bytes_read = std::get<ssize_t>(read_status);
        int first_index = 0;
        auto& unsent_data = consumer->unsent_data;
        for (int i = 0; i < bytes_read; ++i) {
            if (buffer[i] == 0) {
                ReverseEcho(consumer->client, unsent_data);
                unsent_data.clear();
                continue;
            }
            unsent_data.push_back(buffer[i]);
        }
    } while (true);
    return read_status;
}

void HandleClientEvent(events::Epoller& epoller, epoll_event& event)
{
    SPDLOG_INFO("Handling client event");
    net::TcpReadStatus read_status;
    if (event.data.ptr == nullptr) {
        throw std::runtime_error("Failed to handle event, unexpected file descriptor");
    };
    SPDLOG_INFO("Stopped reading/sending data");
    auto status = HandleReadEvent(event);
    return HandleStatus(epoller, event, status);
}

bool HandleDisconnect(epoll_event& event)
{
    auto client = static_cast<net::TcpClient*>(event.data.ptr);
    if (client == nullptr) {
        SPDLOG_ERROR("Unexpected disconnect event with empty socket");
        return;
    }
    int fd = client->GetFd();
    SPDLOG_INFO("Disconnecting fd: {}", fd);
    RemoveTcpSocket(client);
    SPDLOG_INFO("Closed connection for fd {}", fd);
    return false;
}

bool HandleConnect(epoll_event& event)
{
    SPDLOG_INFO("Accepting new connections");
    auto server = static_cast<net::TcpServer*>(event.data.ptr);
    while (true) {
        auto client = server->Accept();
        if (client == nullptr) {
            SPDLOG_INFO("No more consumers available, continue waiting");
            break;
        }
        SPDLOG_INFO("Connection accepted, fd {} has been assigned", client->GetFd());
    }
    return true;
}

} // namespace echo_reverse_server::event_handlers
