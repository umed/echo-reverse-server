#include "event_handlers.hpp"

#include "events/epoller.hpp"
#include "logging.hpp"
#include "net/tcp_client.hpp"
#include "net/tcp_server.hpp"

#include <vector>

namespace echo_reverse_server::event_handlers {

namespace {

using BufferArray = std::array<uint8_t, 1024>;

void CloseConnection(events::Epoller& epoller, FdConsumerMap& consumers, FdConsumerMap::iterator it)
{
    LOG_INFO() << "Closing connection for fd: " << it->first;
    epoller.Remove(it->first);
    consumers.erase(it);
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

void HandleStatus(
    events::Epoller& epoller, FdConsumerMap& consumers, FdConsumerMap::iterator it, const net::ClientStatus status)
{
    switch (status) {
    case net::ClientStatus::Close:
        LOG_INFO() << "ClientStatus::Close";
        CloseConnection(epoller, consumers, std::move(it));
        break;
    case net::ClientStatus::Reschedule:
        LOG_INFO() << "ClientStatus::Reschedule";
        epoller.Rearm(it->first);
        break;
    default:
        LOG_ERROR() << "Unexpected Read's status";
    }
}

} // namespace

bool ShouldCloseConnection(const epoll_event& event)
{
    return (event.events & EPOLLERR) || (event.events & EPOLLHUP) || !(event.events & EPOLLIN);
}

void HandleClientEvent(events::Epoller& epoller, net::TcpServer&, FdConsumerMap& consumers, const epoll_event& event)
{
    LOG_INFO() << "Handling client event";
    auto it = consumers.find(event.data.fd);
    if (it == consumers.end()) {
        throw std::runtime_error("Failed to handle event, unexpected file descriptor");
    }
    BufferArray buffer;
    do {
        net::DataSizeOrClientStatus size_or_status = it->second.client.Read(buffer);
        if (std::holds_alternative<net::ClientStatus>(size_or_status)) {
            auto status = std::get<net::ClientStatus>(size_or_status);
            HandleStatus(epoller, consumers, std::move(it), status);
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
}

void HandleDisconnect(
    events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event)
{
    LOG_INFO() << "Disconnecting fd: " << event.data.fd;
    auto it = consumers.find(event.data.fd);
    if (it != consumers.end()) {
        CloseConnection(epoller, consumers, std::move(it));
    } else {
        LOG_WARNING() << "Going to close connection that is not presented in list of consumers";
    }
    LOG_INFO() << "Closed connection for fd " << event.data.fd;
}

void HandleConnect(events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event)
{
    LOG_INFO() << "Accepting new connection";
    auto client = server.Accept();
    if (!client.has_value()) {
        LOG_INFO() << "No more consumers available, continue waiting";
        return;
    }
    int fd = client->fd;
    consumers.emplace(fd, Consumer { std::move(client.value()), {} });
    epoller.Add(fd);
    LOG_INFO() << "Connection accepted, fd " << fd << " has been assigned";
}

} // namespace echo_reverse_server::event_handlers
