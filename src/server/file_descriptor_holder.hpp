#pragma once

#include <unistd.h>

namespace echo_reverse_server::utils {

struct FileDescriptorHolder {
    constexpr static int INVALID_FILE_DESCRIPTOR = -1;

    FileDescriptorHolder()
        : fd(-1)
    {
    }
    FileDescriptorHolder(int fd)
        : fd(fd)
    {
    }
    FileDescriptorHolder(int& fd)
        : fd(fd)
    {
    }

    FileDescriptorHolder(FileDescriptorHolder&& other)
    {
        this->fd = other.fd;
        other.fd = INVALID_FILE_DESCRIPTOR;
    }

    ~FileDescriptorHolder()
    {
        if (fd > -1) {
            close(fd);
        }
    }

    FileDescriptorHolder& operator=(int fd)
    {
        this->fd = fd;
        return *this;
    }

    operator int() const
    {
        return fd;
    }

private:
    int fd;
};

} // namespace echo_reverse_server::utils
