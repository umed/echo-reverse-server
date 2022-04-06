#include "event_handlers/event_handlers.hpp"
#include "events/epoller.hpp"
#include "logging.hpp"
#include "net/tcp_server.hpp"

using namespace echo_reverse_server;

constexpr uint16_t PORT = 9123;

constexpr int MAX_CONNECTION_NUMBER = 1024;

int main()
{
    net::TcpServer server(PORT, MAX_CONNECTION_NUMBER);
    LOG_INFO() << "Starting server";
    server.Start();

    LOG_INFO() << "Setting up epoll events";
    events::Epoller epoller(server.Connection()->fd);

    event_handlers::FdConsumerMap consumers;
    auto waiter = [&epoller, &server, &consumers](const epoll_event& event) {
        LOG_INFO() << "Event received for fd: " << event.data.fd;
        if (event_handlers::ShouldCloseConnection(event)) {
            event_handlers::HandleDisconnect(epoller, server, consumers, event);
        } else if (event.data.fd == server.Connection()->fd) {
            event_handlers::HandleConnect(epoller, server, consumers, event);
        } else {
            event_handlers::HandleClientEvent(epoller, server, consumers, event);
        }
    };
    LOG_INFO() << "Waiting on epoll";
    epoller.Wait(waiter);
    return EXIT_SUCCESS;
}
