#include "event_handlers.hpp"

#include "events/epoller.hpp"
#include "net/tcp_client.hpp"
#include "net/tcp_server.hpp"

#include <spdlog/spdlog.h>

#include <sys/epoll.h>

#include <vector>

namespace echo_reverse_server::event_handlers {

namespace {

using BufferArray = std::array<uint8_t, 1024>;

void RemoveTcpSocket(net::TcpSocket* tcp_socket)
{
    SPDLOG_INFO("Closing connection for fd: {}", tcp_socket->GetFd());
    delete tcp_socket;
}

void ReverseEcho(const net::TcpClient* client, const std::vector<uint8_t>& unsent_data)
{
    BufferArray reversed_buffer;
    size_t j = 0;
    for (ssize_t i = unsent_data.size() - 1; i > -1;) {
        for (j = 0; j < reversed_buffer.size() - 1 && i > -1; --i, ++j) {
            reversed_buffer[j] = unsent_data[i];
        }
        if (j == reversed_buffer.size()) {
            client->Write(reversed_buffer, j);
        }
    }
    if (j % reversed_buffer.size() != 0) {
        reversed_buffer[j] = 0;
        ++j;
    }
    client->Write(reversed_buffer, j);
}

net::ClientStatus HandleReadEvent(net::TcpClient* client, std::vector<uint8_t>& unsent_data)
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
        SPDLOG_INFO("Read '{}' bytes from client", bytes_read);
        for (int i = 0; i < bytes_read; ++i) {
            if (buffer[i] == 0) {
                ReverseEcho(client, unsent_data);
                unsent_data.clear();
                continue;
            }
            unsent_data.push_back(buffer[i]);
        }
    } while (true);
    return std::get<net::ClientStatus>(read_status);
}

bool ShouldCloseConnection(const uint32_t& events)
{
    return (events & EPOLLERR) || (events & EPOLLHUP) // || (event.events & EPOLLRDHUP)
        || (!(events & EPOLLIN));
}

} // namespace

TcpSocketEventHandler::TcpSocketEventHandler(std::unique_ptr<net::TcpSocket> socket)
    : socket(std::move(socket))
{
}

void TcpSocketEventHandler::Handle(const events::Epoller& epoller, const epoll_event& event)
{
    SPDLOG_INFO("Handling event for fd: '{}'", this->GetFd());
    if (ShouldCloseConnection(event.events)) {
        SPDLOG_INFO("Closing connection for fd: {}", socket->GetFd());
        delete this;
    } else if (HandleImpl(epoller)) {
        SPDLOG_INFO("Rearming connection for fd: {}", socket->GetFd());
        epoller.Rearm(this);
    } else {
        SPDLOG_INFO("Closing connection for fd: {}", socket->GetFd());
        delete this;
    }
}

int TcpSocketEventHandler::GetFd()
{
    return socket->GetFd();
}

bool ClientEventHandler::HandleImpl(const events::Epoller& epoller)
{
    SPDLOG_INFO("Handling client event: {}", this->GetFd());
    auto client = static_cast<net::TcpClient*>(socket.get());
    if (!client) {
        throw std::runtime_error("Failed to handle event, unexpected socket");
    };
    net::TcpReadStatus read_status;
    return event_handlers::HandleReadEvent(client, unsent_data) == net::ClientStatus::Reschedule;
}

bool ServerEventHandler::HandleImpl(const events::Epoller& epoller)
{
    SPDLOG_INFO("Accepting new connections");
    auto server = static_cast<net::TcpServer*>(socket.get());
    while (true) {
        auto client = server->Accept();
        if (!client) {
            SPDLOG_INFO("No more consumers available, continue waiting");
            break;
        }
        SPDLOG_INFO("Connection accepted, fd {} has been assigned", client->GetFd());
        epoller.Add(new ClientEventHandler(std::move(client)));
    }
    return true;
}

} // namespace echo_reverse_server::event_handlers
