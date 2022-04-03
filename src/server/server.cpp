#include "logging.hpp"

#include "tcp_server.hpp"

#include <algorithm>
#include <map>

using namespace echo_reverse_server;

constexpr uint16_t PORT = 9123;

constexpr int MAX_CONNECTION_NUMBER = 1024;

#include <sys/epoll.h>
#include <vector>

#include "exceptions.hpp"

namespace {

constexpr int BLOCK_WAIT_INDEFINITLY = -1;

void Initialize(int epoll_fd, int listen_socket)
{
    epoll_event event;
    event.data.fd = listen_socket;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &event) == -1) {
        throw KernelError("Failed to initialize accept");
    }
}

using EpollEvents = std::vector<epoll_event>;

} // namespace

// struct EpollEventConsumer {
//     void
// };

struct DataReader {
};

struct Epoller {
    Epoller(net::ConnectionPtr connection, int max_events = 24)
        : fd(epoll_create1(0))
        , connection(std::move(connection))
        , max_events(max_events)
    {
        if (fd == utils::FileDescriptorHolder::INVALID_FILE_DESCRIPTOR) {
            throw KernelError("Failed to create epoll instance");
        }
        if (max_events < 0) {
            throw KernelError("Max events size must be bigger than 0");
        }
    }

    using EventHandler = std::function<void(const epoll_event&)>;

    void Poll(const EventHandler& connect_handler, const EventHandler& data_handler)
    {
        Initialize(fd, connection->fd);
        for (;;) {
            EpollEvents events(max_events);
            int events_received = Wait(events);
            HandleEvents(events, events_received, connect_handler, data_handler);
        }
    }

    void HandleEvents(
        const EpollEvents& events, int size, const EventHandler& connect_handler, const EventHandler& data_handler)
    {
        std::for_each(events.begin(), events.begin() + size, [&](const auto& event) {
            if (event.data.fd == connection->fd) {
                connect_handler(event);
            } else {
                data_handler(event);
            }
        });
    }

    int Wait(EpollEvents& events, bool untill_event = true)
    {

        int events_number;
        do {
            events_number = epoll_wait(fd, events.data(), events.size(), BLOCK_WAIT_INDEFINITLY);
            if (events_number == -1) {
                throw KernelError("Failed to wait on epoll");
            }
        } while (untill_event && events_number == 0);
        return events_number;
    }

    void Append(int new_fd, void* data, bool restart = false)
    {
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLONESHOT;
        event.data.ptr = data;
        int action = restart ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        int result = epoll_ctl(fd, action, new_fd, &event);
        if (result == -1) {
            throw KernelError("Failed to add to listen list");
        }
    }

    void Rearm(int old_fd, void* data)
    {
        Append(old_fd, data, true);
    }

    utils::FileDescriptorHolder fd;

    net::ConnectionPtr connection;
    int max_events;
};

int main()
{
    net::TcpServer server(PORT, MAX_CONNECTION_NUMBER);
    server.Start();

    Epoller epoller(server.Connection());
    auto client = server.Accept();

    std::map<int, net::TcpClient> clients;
    epoller.Poll(
        [&epoller, &server, &clients](const auto& event) {
            auto client = server.Accept();
            int fd = client.fd;
            clients.emplace(fd, std::move(client));
            epoller.Append(client.fd, static_cast<void*>(&clients[fd]));
        },
        [](const auto& event) {});
    std::array<uint8_t, 1024> buffer;
    size_t bytes_read = client.Recveive(buffer);

    std::reverse(buffer.begin(), buffer.begin() + bytes_read);

    client.Send(buffer, bytes_read);
    LOG_INFO() << "Success" << bytes_read;
    for (int i = 0; i < bytes_read; ++i)
        std::cout << std::hex << static_cast<unsigned>(buffer[i]) << ' ';
    std::cout << std::endl;
    LOG_INFO() << "Finish";
    return EXIT_SUCCESS;
}
