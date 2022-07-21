#pragma once

#include "utils/fd_helpers.hpp"

#include <sys/epoll.h>

#include <functional>

namespace echo_reverse_server::events {

constexpr auto MAX_EVENT_NUMBER = 64;

bool ShouldWaitForNewEvents();

struct EpollData {
    int fd;
    void* data;
};

struct Epoller {
    using EventHandler = std::function<bool(const epoll_event&)>;

    Epoller(int connection_fd, int max_events = MAX_EVENT_NUMBER);

    void Wait(const EventHandler& event_consumer);

    void Add(int fd_to_add, void* data = nullptr);

    void Remove(int fd_to_remove);

    void Rearm(int old_fd, void* data = nullptr);

    ~Epoller()
    {
        utils::Close(epoll_fd);
    }

private:
    int epoll_fd;
    int max_events;
};

} // namespace echo_reverse_server::events
