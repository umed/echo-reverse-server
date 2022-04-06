#pragma once

#include "utils/exceptions.hpp"
#include "utils/file_descriptor_holder.hpp"

#include <array>
#include <variant>

#include <sys/socket.h>
#include <sys/types.h>

namespace echo_reverse_server::net {

enum class ClientStatus { Reschedule, Close };

using DataSizeOrClientStatus = std::variant<ssize_t, ClientStatus>;

struct TcpClient {

    TcpClient(utils::FileDescriptorHolder&& fd)
        : fd(std::move(fd))
    {
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
        send(fd, buffer.data(), count, 0);
    }

    utils::FileDescriptorHolder fd;
};

} // namespace echo_reverse_server::net
