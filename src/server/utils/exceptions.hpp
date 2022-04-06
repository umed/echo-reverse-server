#pragma once

#include <fmt/format.h>

#include <cstring>
#include <exception>

namespace echo_reverse_server {

class KernelError : std::exception {
public:
    KernelError()
        : message(std::strerror(errno))
    {
    }
    KernelError(std::string_view error_message)
        : message(fmt::format("{}: {}", error_message, std::strerror(errno)))
    {
    }

    const char* what() const noexcept override
    {
        return message.c_str();
    }

private:
    std::string message;
};

} // namespace echo_reverse_server
