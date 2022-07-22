#pragma once

#include <memory>
#include <vector>

#include "events/epoller.hpp"

struct epoll_event;

namespace echo_reverse_server::net {

struct TcpSocket;

} // namespace echo_reverse_server::net

namespace echo_reverse_server::event_handlers {


struct TcpSocketEventHandler : public events::EventWrapper {
    TcpSocketEventHandler(std::unique_ptr<net::TcpSocket> socket) ;
    virtual ~TcpSocketEventHandler() = default;
    int GetFd();

    /**
     * @brief handles memory cleanup of current event handler
     */
    void Handle(const events::Epoller& epoller, const epoll_event& event);

protected:
    std::unique_ptr<net::TcpSocket> socket;

private:
    /**
     * @brief Epoller event handler type
     *
     * @param epoller
     * @param event
     * @return true if event handler need to be rearmed
     * @return false if rearming is not needed
     */
    virtual bool HandleImpl(const events::Epoller& epoller) = 0;
};

struct ClientEventHandler final : public TcpSocketEventHandler {
    using TcpSocketEventHandler::TcpSocketEventHandler;

private:
    bool HandleImpl(const events::Epoller& epoller) override;

    std::vector<uint8_t> unsent_data;
};

struct ServerEventHandler final : public TcpSocketEventHandler {
    using TcpSocketEventHandler::TcpSocketEventHandler;

private:
    bool HandleImpl(const events::Epoller& epoller) override;
};

} // namespace echo_reverse_server::event_handlers
