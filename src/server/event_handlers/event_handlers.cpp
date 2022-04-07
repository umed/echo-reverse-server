#include "event_handlers.hpp"

#include "events/epoller.hpp"
#include "net/tcp_client.hpp"
#include "net/tcp_server.hpp"

#include <spdlog/spdlog.h>

#include <vector>

namespace echo_reverse_server::event_handlers {

namespace {

using BufferArray = std::array<uint8_t, 1024>;

void CloseConnection(events::Epoller& epoller, SynchrnoziedConsumers& consumers, FdConsumerMap::iterator it)
{
    SPDLOG_INFO("Closing connection for fd: {}", it->first);
    epoller.Remove(it->first);
    consumers.data.erase(it);
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

void HandleStatus(events::Epoller& epoller,
    SynchrnoziedConsumers& consumers,
    FdConsumerMap::iterator it,
    const net::ClientStatus status)
{
    SPDLOG_INFO("Handling net::ClientStatus");
    switch (status) {
    case net::ClientStatus::Close:
        SPDLOG_INFO("ClientStatus::Close");
        CloseConnection(epoller, consumers, std::move(it));
        break;
    case net::ClientStatus::Reschedule:
        SPDLOG_INFO("ClientStatus::Reschedule");
        epoller.Rearm(it->first);
        break;
    default:
        SPDLOG_ERROR("Unexpected Read's status");
    }
}

} // namespace

bool ShouldCloseConnection(const epoll_event& event)
{
    return (event.events & EPOLLERR) || (event.events & EPOLLHUP) || (event.events & EPOLLRDHUP)
        || !(event.events & EPOLLIN);
}

void HandleClientEvent(
    events::Epoller& epoller, net::TcpServer&, SynchrnoziedConsumers& consumers, const epoll_event& event)
{
    SPDLOG_INFO("Handling client event");
    net::DataSizeOrClientStatus size_or_status;
    {
        std::shared_lock lock(consumers.mutex);
        auto it = consumers.data.find(event.data.fd);
        if (it == consumers.data.end()) {
            throw std::runtime_error("Failed to handle event, unexpected file descriptor");
        }
        BufferArray buffer;
        do {
            size_or_status = it->second.client.Read(buffer);
            if (std::holds_alternative<net::ClientStatus>(size_or_status)) {
                SPDLOG_INFO("Has got net::ClientStatus"); // {}", static_cast<int>(net::ClientStatus));
                break;
            }
            auto& consumer = it->second;
            auto bytes_read = std::get<ssize_t>(size_or_status);
            int first_index = 0;
            auto& unsent_data = consumer.unsent_data;
            for (int i = 0; i < bytes_read; ++i) {
                if (buffer[i] == 0) {
                    ReverseEcho(consumer.client, unsent_data);
                    unsent_data.clear();
                    continue;
                }
                unsent_data.push_back(buffer[i]);
            }
        } while (true);
        SPDLOG_INFO("Stopped reading/sending data");
    }
    {
        std::unique_lock lock(consumers.mutex);
        auto it = consumers.data.find(event.data.fd);
        if (it == consumers.data.end()) {
            throw std::runtime_error("Failed to handle event, unexpected file descriptor");
        }
        auto status = std::get<net::ClientStatus>(size_or_status);
        HandleStatus(epoller, consumers, std::move(it), status);
    }
}

void HandleDisconnect(events::Epoller& epoller,
    net::TcpServer& server,
    SynchrnoziedConsumers& consumers,
    const epoll_event& event) noexcept
{
    SPDLOG_INFO("Disconnecting fd: {}", event.data.fd);
    {
        std::unique_lock lock(consumers.mutex);
        auto it = consumers.data.find(event.data.fd);
        if (it != consumers.data.end()) {
            CloseConnection(epoller, consumers, std::move(it));
        } else {
            SPDLOG_WARN("Going to close connection that is not presented in list of consumers");
        }
    }
    SPDLOG_INFO("Closed connection for fd {}", event.data.fd);
}

void HandleConnect(
    events::Epoller& epoller, net::TcpServer& server, SynchrnoziedConsumers& consumers, const epoll_event& event)
{
    SPDLOG_INFO("Accepting new connection");
    auto client = server.Accept();
    if (!client.has_value()) {
        SPDLOG_INFO("No more consumers available, continue waiting");
        return;
    }
    int fd = client->fd;
    {
        std::unique_lock lock(consumers.mutex);
        consumers.data.emplace(fd, Consumer { std::move(client.value()), {} });
    }
    epoller.Add(fd);
    SPDLOG_INFO("Connection accepted, fd {} has been assigned", fd);
}

} // namespace echo_reverse_server::event_handlers
