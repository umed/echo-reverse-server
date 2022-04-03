#pragma once

#include "exceptions.hpp"
#include "file_descriptor_holder.hpp"

#include <array>
#include <cstring>

#include <sys/socket.h>
#include <sys/types.h>

namespace echo_reverse_server::net {

class TcpClient {
public:
    TcpClient(utils::FileDescriptorHolder&& fd)
        : fd(std::move(fd))
    {
    }

    template <size_t buffer_size>
    ssize_t Recveive(std::array<uint8_t, buffer_size>& buffer)
    {
        ssize_t bytes_read = recv(fd, buffer.data(), buffer.size(), 0);
        if (bytes_read < 0) {
            if (errno == EAGAIN) {
                return 0;
            }
            throw KernelError("Failed to read bytes");
        }
        return bytes_read;
    }

    template <size_t buffer_size>
    void Send(std::array<uint8_t, buffer_size>& buffer, size_t count)
    {
        send(fd, buffer.data(), count, 0);
    }

    utils::FileDescriptorHolder fd;
};

} // namespace echo_reverse_server::net
