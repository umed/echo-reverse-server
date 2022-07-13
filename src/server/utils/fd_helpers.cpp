#include "fd_helpers.hpp"

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

bool IsValid(int fd)
{
    return fd > 0;
}

void Close(int fd)
{
    close(fd);
}

} // namespace echo_reverse_server::utils
