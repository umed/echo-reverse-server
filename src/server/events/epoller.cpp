#include "epoller.hpp"

#include "event_handlers/event_handlers.hpp"
#include "tcp_socket.hpp"
#include "utils/exceptions.hpp"

#include <spdlog/spdlog.h>

#include <vector>

namespace echo_reverse_server::events {

namespace {

using EpollEvents = std::vector<epoll_event>;

constexpr int BLOCK_WAIT_INDEFINITLY = -1;

void HandleEvents(Epoller* epoller, const EpollEvents& events, int size, const Epoller::EventHandler& event_consumer)
{
    for (int i = 0; i < size; ++i) {
        auto& event = events[i];
        if (event_consumer(event)) {
            net::TcpSocket* socket = static_cast<net::TcpSocket*>(event.data.ptr);
            epoller->Rearm(socket->GetFd(), event.data.ptr);
        }
    }
}

int RunWaitLoop(int epoll_fd, EpollEvents& events, bool untill_event = true)
{
    int events_number;
    do {
        events_number = epoll_wait(epoll_fd, events.data(), events.size(), BLOCK_WAIT_INDEFINITLY);
        if (events_number == -1 && errno != EINTR) {
            throw KernelError("Failed to wait on epoll");
        }
    } while (untill_event && events_number <= 0);
    return events_number;
}

} // namespace

bool ShouldWaitForNewEvents()
{
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

Epoller::Epoller(int listen_socket_fd, int max_events)
    : epoll_fd(epoll_create1(0))
    , max_events(max_events)
{
    if (epoll_fd == -1) {
        throw KernelError("Failed to create epoll instance");
    }
    if (max_events < 0) {
        throw KernelError("Max events size must be bigger than 0");
    }

    epoll_event event;
    event.data.ptr = new event_handlers::Consumer { { listen_socket_fd }, {} };
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLPRI | EPOLLERR;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &event) == -1) {
        throw KernelError("Failed to initialize accept");
    }
}

void Epoller::Wait(const EventHandler& event_consumer)
{
    EpollEvents events(max_events);
    for (;;) {
        try {
            int events_received = RunWaitLoop(epoll_fd, events);
            HandleEvents(this, events, events_received, event_consumer);
        } catch (const KernelError& e) {
            SPDLOG_ERROR(e.what());
        }
    }
}

void Epoller::Add(int fd_to_add, void* data)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = data;
    utils::SetNonBlocking(fd_to_add);
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_to_add, &event);
    if (result == -1) {
        throw KernelError("Failed to add file descriptor to subscription list");
    }
}

void Epoller::Remove(int fd_to_remove)
{
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_to_remove, NULL);
    if (result == -1) {
        throw KernelError("Failed to remove file descriptor from subscription list");
    }
}

void Epoller::Rearm(int old_fd, void* data)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = data;
    utils::SetNonBlocking(old_fd);
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, old_fd, &event);
    if (result == -1) {
        throw KernelError("Failed to add file descriptor to subscription list");
    }
}

} // namespace echo_reverse_server::events
