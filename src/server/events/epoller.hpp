#pragma once

#include "utils/file_descriptor_holder.hpp"

#include <sys/epoll.h>

#include <functional>

namespace echo_reverse_server::events {

bool ShouldWaitForNewEvents();

struct Epoller {
    using EventHandler = std::function<void(const epoll_event&)>;

    Epoller(int connection_fd, int max_events = 64);

    void Wait(const EventHandler& event_consumer);

    void Add(int fd_to_add, bool restart = false, void* data = nullptr);

    void Remove(int fd_to_remove);

    void Rearm(int old_fd, void* data = nullptr);

private:
    utils::FileDescriptorHolder fd;
    int max_events;
};

} // namespace echo_reverse_server::events
