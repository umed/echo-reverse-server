#pragma once

#include <unistd.h>

namespace echo_reverse_server::utils {

struct FileDescriptorHolder {
    constexpr static int INVALID_FILE_DESCRIPTOR = -1;

    FileDescriptorHolder()
        : fd(INVALID_FILE_DESCRIPTOR)
    {
    }

    FileDescriptorHolder(const FileDescriptorHolder&) = delete;
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

    bool IsValid()
    {
        return fd > 0;
    }

    int Release()
    {
        int return_fd = fd;
        fd = INVALID_FILE_DESCRIPTOR;
        return return_fd;
    }

    ~FileDescriptorHolder()
    {
        if (IsValid()) {
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
