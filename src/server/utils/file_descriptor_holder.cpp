#include "file_descriptor_holder.hpp"

#include "exceptions.hpp"

#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace echo_reverse_server::utils {

bool WouldBlock()
{
    SPDLOG_INFO("Would block: {}", std::strerror(errno));
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

void SetNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw KernelError("Failed to acquire flags of new file descriptor");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw KernelError("Failed to make file descriptor non blocking");
    }
}

FileDescriptorHolder::FileDescriptorHolder()
    : fd(INVALID_FILE_DESCRIPTOR)
{
}

FileDescriptorHolder::FileDescriptorHolder(int fd)
    : fd(fd)
{
}
FileDescriptorHolder::FileDescriptorHolder(int& fd)
    : fd(fd)
{
}

FileDescriptorHolder::FileDescriptorHolder(FileDescriptorHolder&& other)
{
    this->fd = other.fd;
    other.fd = INVALID_FILE_DESCRIPTOR;
}

bool FileDescriptorHolder::IsValid()
{
    return fd > 0;
}

int FileDescriptorHolder::Release()
{
    int return_fd = fd;
    fd = INVALID_FILE_DESCRIPTOR;
    return return_fd;
}

FileDescriptorHolder::~FileDescriptorHolder()
{
    if (IsValid()) {
        close(fd);
    }
}

FileDescriptorHolder& FileDescriptorHolder::operator=(int fd)
{
    this->fd = fd;
    return *this;
}

FileDescriptorHolder::operator int() const
{
    return fd;
}

} // namespace echo_reverse_server::utils
