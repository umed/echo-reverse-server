#pragma once

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
    size_t Read(std::array<uint8_t, buffer_size>& buffer)
    {
        // TODO: user errno
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        // ssize_t bytes_read = recv(fd, buffer.data(), buffer.size(), 0);
        if (bytes_read < 0) {
            LOG_ERROR() << std::strerror(errno);
            throw std::runtime_error("Failed to read bytes");
        }
        return bytes_read;
    }

    template <size_t buffer_size>
    void Send(std::array<uint8_t, buffer_size>& buffer, size_t count)
    {
        send(fd, buffer.data(), count, 0);
    }

private:
    utils::FileDescriptorHolder fd;
};

} // namespace echo_reverse_server::net
