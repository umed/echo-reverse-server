#include "epoller.hpp"

#include "net/tcp_socket.hpp"
#include "utils/exceptions.hpp"
#include "utils/fd_helpers.hpp"

#include <spdlog/spdlog.h>

#include <sys/epoll.h>

#include <vector>

namespace echo_reverse_server::events {

namespace {

using EpollEvents = std::vector<epoll_event>;

constexpr int BLOCK_WAIT_INDEFINITLY = -1;

void HandleEvents(Epoller& epoller, const EpollEvents& events, int size, const Epoller::EventHandler& event_handler)
{
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

Epoller::Epoller(EventWrapper* event_wrapper, int max_events)
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
    event.data.ptr = static_cast<void*>(event_wrapper);
    // EPOLLONESHOT, indicates that we will have to read and wait for available data
    // until EAGAIN or EWOULDBLOCK is returned
    event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLET | EPOLLONESHOT;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_wrapper->GetFd(), &event) == -1) {
        throw KernelError("Failed to subscribe during epoller initialization");
    }
}

Epoller::~Epoller()
{
    utils::Close(epoll_fd);
}

void Epoller::Wait(const EventHandler& event_handler) const
{
    EpollEvents events(max_events);
    for (;;) {
        try {
            int events_received = RunWaitLoop(epoll_fd, events);
            for (int i = 0; i < events_received; ++i) {
                auto& event = events[i];
                event_handler(*this, event);
            }
        } catch (const KernelError& e) {
            SPDLOG_ERROR(e.what());
        }
    }
}

void Epoller::Add(EventWrapper* event_wrapper) const
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.ptr = static_cast<void*>(event_wrapper);
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_wrapper->GetFd(), &event);
    if (result == -1) {
        throw KernelError("Failed to add file descriptor to subscription list");
    }
}

void Epoller::Remove(EventWrapper* event_wrapper) const
{
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_wrapper->GetFd(), NULL);
    if (result == -1) {
        throw KernelError("Failed to remove file descriptor from subscription list");
    }
}

void Epoller::Rearm(EventWrapper* event_wrapper) const
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.ptr = static_cast<void*>(event_wrapper);
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_wrapper->GetFd(), &event);
    if (result == -1) {
        throw KernelError("Failed to add file descriptor to subscription list");
    }
}

} // namespace echo_reverse_server::events
