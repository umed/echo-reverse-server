#include "consumer.hpp"

#include "epoller.hpp"
#include "logging.hpp"

#include <algorithm>

namespace echo_reverse_server {

namespace {

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

} // namespace

void Consumer::Consume(
    events::Epoller& epoller, net::TcpServer& server, FdConsumerMap& consumers, const epoll_event& event)
{

    auto it = consumers.find(event.data.fd);
    if (it == consumers.end()) {
        throw std::runtime_error("Failed to handle event, unexpected file descriptor");
    }
    std::array<uint8_t, 1024> buffer;
    do {
        net::DataSizeOrClientStatus size_or_status = it->second.Read(buffer);
        if (std::holds_alternative<net::ClientStatus>(size_or_status)) {
            auto status = std::get<net::ClientStatus>(size_or_status);
            status_handler(epoller, consumers, std::move(it), status);
            return;
        } else {
            auto bytes_read = std::get<ssize_t>(size_or_status);
            std::reverse(buffer.begin(), buffer.begin() + bytes_read);
        }
    } while (true);
}

} // namespace echo_reverse_server
