
#include "epoller.hpp"
#include "logging.hpp"
#include "tcp_client.hpp"
#include "tcp_server.hpp"

#include <map>
#include <vector>

using namespace echo_reverse_server;

constexpr uint16_t PORT = 9123;

constexpr int MAX_CONNECTION_NUMBER = 1024;

struct Consumer {
    net::TcpClient client;
    std::vector<uint8_t> unsent_data;
};

using FdConsumerMap = std::map<int, Consumer>;

using BufferArray = std::array<uint8_t, 1024>;

bool ShouldCloseConnection(const epoll_event& event)
{
    return (event.events & EPOLLERR) || (event.events & EPOLLHUP) || !(event.events & EPOLLIN);
}

void CloseConnection(events::Epoller& epoller, FdConsumerMap& consumers, FdConsumerMap::iterator it)
{
    epoller.Remove(it->first);
    consumers.erase(it);
}

void HandleStatus(
    events::Epoller& epoller, FdConsumerMap& consumers, FdConsumerMap::iterator it, const net::ClientStatus status)
{
    switch (status) {
    case net::ClientStatus::Close:
        CloseConnection(epoller, consumers, std::move(it));
        break;
    case net::ClientStatus::Reschedule:
        epoller.Rearm(it->first);
        break;
    default:
        LOG_ERROR() << "Unexpected Read's status";
    }
}

void ReverseEcho(const net::TcpClient& client, const std::vector<uint8_t>& unsent_data)
{
    BufferArray reversed_buffer;
    size_t counter = 0;
    for (size_t i = unsent_data.size() - 1; i >= 0; --i) {
        size_t j = 0;
        for (; j < reversed_buffer.size() && i >= 0; --i, ++j) {
            reversed_buffer[j] = unsent_data[i];
        }
        client.Write(reversed_buffer, j);
    }
    reversed_buffer.front() = 0;
    client.Write(reversed_buffer, 1);
}

void HandleClientEvent(
    events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event)
{

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
    auto client = server.Accept();
    if (!client.has_value()) {
        LOG_INFO() << "No more consumers available, continue waiting";
        return;
    }
    int fd = client->fd;
    consumers.emplace(fd, Consumer { std::move(client.value()), {} });
    epoller.Add(fd);
}

int main()
{
    net::TcpServer server(PORT, MAX_CONNECTION_NUMBER);
    server.Start();

    events::Epoller epoller(server.Connection()->fd);

    FdConsumerMap consumers;
    epoller.Poll(server.Connection()->fd, [&epoller, &server, &consumers](const epoll_event& event) {
        if (ShouldCloseConnection(event)) {
            HandleDisconnect(epoller, server, consumers, event);
        } else if (event.data.fd == server.Connection()->fd) {
            HandleConnect(epoller, server, consumers, event);
        } else {
            HandleClientEvent(epoller, server, consumers, event);
        }
    });
    return EXIT_SUCCESS;
}
