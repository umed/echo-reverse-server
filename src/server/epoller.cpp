#include "epoller.hpp"

#include "exceptions.hpp"
#include "logging.hpp"

#include <vector>

namespace echo_reverse_server::events {

namespace {

using EpollEvents = std::vector<epoll_event>;

constexpr int BLOCK_WAIT_INDEFINITLY = -1;

void Initialize(int epoll_fd, int listen_socket)
{
    epoll_event event;
    event.data.fd = listen_socket;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &event) == -1) {
        throw KernelError("Failed to initialize accept");
    }
}

void HandleEvents(const EpollEvents& events, int size, const Epoller::EventHandler& event_consumer)
{
    std::for_each(events.begin(), events.begin() + size, [&](const epoll_event& event) {
        event_consumer(event);
    });
}

int Wait(int fd, EpollEvents& events, bool untill_event = true)
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

} // namespace

bool ShouldWaitForNewEvents()
{
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

Epoller::Epoller(int max_events)
    : fd(epoll_create1(0))
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

void Epoller::Poll(int connection_fd, const EventHandler& event_consumer)
{
    Initialize(fd, connection_fd);
    EpollEvents events(max_events);
    for (;;) {
        int events_received = Wait(fd, events);
        HandleEvents(std::move(events), events_received, event_consumer);
    }
}

void Epoller::Add(int fd_to_add, bool restart, void* data)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    if (data)
        event.data.ptr = data;
    else
        event.data.fd = fd_to_add;
    int action = restart ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    utils::SetNonBlocking(fd_to_add);
    int result = epoll_ctl(fd, action, fd_to_add, &event);
    if (result == -1) {
        throw KernelError("Failed to add file descriptor to subscription list");
    }
}

void Epoller::Remove(int fd_to_remove)
{
    utils::SetNonBlocking(fd_to_remove);
    int result = epoll_ctl(fd, EPOLL_CTL_DEL, fd_to_remove, NULL);
    if (result == -1) {
        throw KernelError("Failed to remove file descriptor from subscription list");
    }
}

void Epoller::Rearm(int old_fd, void* data)
{
    Add(old_fd, true, data);
}

} // namespace echo_reverse_server::events
