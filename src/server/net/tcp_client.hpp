#pragma once

#include "utils/exceptions.hpp"
#include "utils/fd_helpers.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <array>
#include <stdexcept>
#include <string_view>
#include <variant>

#include <sys/socket.h>
#include <sys/types.h>

namespace echo_reverse_server::net {

struct ConnectionLost : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum class ClientStatus { Reschedule, Close };

using DataSizeOrClientStatus = std::variant<ssize_t, ClientStatus>;

struct TcpClient {

    TcpClient(int fd)
        : fd(fd)
    {
    }

    ~TcpClient()
    {
        utils::Close(fd);
    }

    template <size_t buffer_size>
    DataSizeOrClientStatus Read(std::array<uint8_t, buffer_size>& buffer) const
    {
        ssize_t bytes_read = recv(fd, buffer.data(), buffer.size(), 0);
        if (bytes_read < 0) {
            if (utils::WouldBlock()) {
                return ClientStatus::Reschedule;
            }
            throw KernelError("Failed to read bytes");
        } else if (bytes_read == 0) {
            return ClientStatus::Close;
        }
        return bytes_read;
    }

    template <size_t buffer_size>
    void Write(std::array<uint8_t, buffer_size>& buffer, size_t count) const
    {
        ssize_t bytes_sent = send(fd, buffer.data(), count, MSG_NOSIGNAL);
        if (bytes_sent < 0) {
            if (errno == EPIPE) {
                throw ConnectionLost(
                    fmt::format("Failed to send data, connection has been lost. errno: {}", std::strerror(errno)));
            } else {
                throw ConnectionLost(
                    fmt::format("Failed to send data, unexpected error. errno: {}", std::strerror(errno)));
            }
        }
    }

    int fd;
};

} // namespace echo_reverse_server::net
