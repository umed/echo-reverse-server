#include "event_handlers.hpp"

#include "events/epoller.hpp"
#include "net/tcp_client.hpp"
#include "net/tcp_server.hpp"

#include <spdlog/spdlog.h>

#include <vector>

namespace echo_reverse_server::event_handlers {

namespace {

using BufferArray = std::array<uint8_t, 1024>;

void CloseConsumer(events::Epoller& epoller, Consumer* consumer)
{
    SPDLOG_INFO("Closing connection for fd: {}", consumer->client.fd);
    epoller.Remove(consumer->client.fd);
    delete consumer;
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

void HandleStatus(events::Epoller& epoller, Consumer* consumer, const net::ClientStatus status)
{
    SPDLOG_INFO("Handling net::ClientStatus");
    switch (status) {
    case net::ClientStatus::Close:
        SPDLOG_INFO("ClientStatus::Close");
        CloseConsumer(epoller, consumer);
        break;
    case net::ClientStatus::Reschedule:
        SPDLOG_INFO("ClientStatus::Reschedule");
        epoller.Rearm(consumer->client.fd, static_cast<void*>(consumer));
        break;
    default:
        SPDLOG_ERROR("Unexpected Read's status");
    }
}

} // namespace

bool ShouldCloseConnection(const epoll_event& event)
{
    return (event.events & EPOLLERR) || (event.events & EPOLLHUP) || (event.events & EPOLLRDHUP)
        || (!(event.events & EPOLLIN));
}

void HandleClientEvent(events::Epoller& epoller, net::TcpServer&, const epoll_event& event)
{
    SPDLOG_INFO("Handling client event");
    net::DataSizeOrClientStatus size_or_status;
    auto consumer = static_cast<Consumer*>(event.data.ptr);
    if (consumer == nullptr) {
        throw std::runtime_error("Failed to handle event, unexpected file descriptor");
    }
    BufferArray buffer;
    do {
        size_or_status = consumer->client.Read(buffer);
        if (std::holds_alternative<net::ClientStatus>(size_or_status)) {
            SPDLOG_INFO("Has got net::ClientStatus");
            break;
        }
        auto bytes_read = std::get<ssize_t>(size_or_status);
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
    SPDLOG_INFO("Stopped reading/sending data");
    auto status = std::get<net::ClientStatus>(size_or_status);
    HandleStatus(epoller, consumer, status);
}

void HandleDisconnect(events::Epoller& epoller, net::TcpServer& server, const epoll_event& event) noexcept
{
    auto consumer = static_cast<Consumer*>(event.data.ptr);
    if (consumer == nullptr) {
        SPDLOG_ERROR("Unexpected disconnect event with empty data");
        return;
    }
    int fd = consumer->client.fd;
    SPDLOG_INFO("Disconnecting fd: {}", fd);
    CloseConsumer(epoller, consumer);
    SPDLOG_INFO("Closed connection for fd {}", fd);
}

void HandleConnect(events::Epoller& epoller, net::TcpServer& server, const epoll_event& event)
{
    SPDLOG_INFO("Accepting new connection");
    auto client = server.Accept();
    if (!client.has_value()) {
        SPDLOG_INFO("No more consumers available, continue waiting");
        return;
    }
    int fd = client->fd;
    auto consumer = new Consumer { std::move(client.value()), {} };
    epoller.Add(fd, false, static_cast<void*>(consumer));
    SPDLOG_INFO("Connection accepted, fd {} has been assigned", fd);
}

} // namespace echo_reverse_server::event_handlers
