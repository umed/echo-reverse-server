#pragma once

#include "tcp_socket.hpp"
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

using TcpReadStatus = std::variant<ssize_t, ClientStatus>;

struct TcpClient : public TcpSocket {

    TcpClient(int fd)
        : TcpSocket(fd)
    {
    }

    template <size_t buffer_size>
    TcpReadStatus Read(std::array<uint8_t, buffer_size>& buffer) const
    {
        ssize_t bytes_read = recv(fd, buffer.data(), buffer.size(), 0);
        if (bytes_read < 0) {
            if (utils::WouldBlock()) {
                SPDLOG_INFO("Client will be rescheduled");
                return ClientStatus::Reschedule;
            }
            throw KernelError(fmt::format("Failed to read from {} fd", fd));
        } else if (bytes_read == 0) {
            SPDLOG_INFO("Client will be closed");
            return ClientStatus::Close;
        }
        return bytes_read;
    }

    template <size_t buffer_size>
    void Write(std::array<uint8_t, buffer_size>& buffer, size_t count) const
    {
        ssize_t bytes_sent = 0;
        do {
            count -= bytes_sent;
            bytes_sent = send(fd, buffer.data() + bytes_sent, count, MSG_NOSIGNAL);
            if (bytes_sent < 0) {
                if (errno == EPIPE) {
                    throw ConnectionLost(
                        fmt::format("Failed to send data, connection has been lost. errno: {}", std::strerror(errno)));
                } else {
                    throw ConnectionLost(
                        fmt::format("Failed to send data, unexpected error. errno: {}", std::strerror(errno)));
                }
            }
        } while (bytes_sent < count);
    }
};

} // namespace echo_reverse_server::net
