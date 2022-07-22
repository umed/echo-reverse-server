#pragma once

#include <functional>

struct epoll_event;

namespace echo_reverse_server::events {

constexpr auto MAX_EVENT_NUMBER = 64;

struct EventWrapper {
    virtual ~EventWrapper() = default;

    virtual int GetFd() = 0;
};

struct Epoller {
    using EventHandler = std::function<void(const Epoller& epoller, const epoll_event&)>;

    Epoller(EventWrapper* event_handler, int max_events = MAX_EVENT_NUMBER);
    ~Epoller();

    void Wait(const EventHandler& event_consumer) const;

    void Add(EventWrapper* event_wrapper) const;
    void Remove(EventWrapper* event_wrapper) const;
    void Rearm(EventWrapper* event_wrapper) const;

private:
    int epoll_fd;
    int max_events;
};

} // namespace echo_reverse_server::events
