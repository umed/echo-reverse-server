#pragma once

#include "utils/fd_helpers.hpp"

#include <stdexcept>

namespace echo_reverse_server::net {

struct TcpSocket {
    TcpSocket(int fd)
        : fd(fd)
    {
        if (!utils::IsValid(fd)) {
            throw std::runtime_error("Invalid socket");
        }
        utils::SetNonBlocking(fd);
    }

    virtual ~TcpSocket()
    {
        utils::Close(fd);
    }

    int GetFd() const
    {
        return fd;
    }

protected:
    int fd;
};

} // namespace echo_reverse_server::net
