#include "logging.hpp"

#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <array>
#include <cstdlib>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr int PORT = 9123;

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        LOG_CRITICAL() << "Failed to create socket";
        return EXIT_FAILURE;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        LOG_CRITICAL() << "\nInvalid address/ Address not supported \n";
        return -1;
    }
    if (connect(server_fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        LOG_CRITICAL() << "Failed to listen for connections";
        return EXIT_FAILURE;
    }
    uint8_t data[] = { 1, 2, 3, 0, 1, 4, 3, 0 };
    send(server_fd, data, sizeof(data), 0);
    LOG_INFO() << "Message sent";
    uint8_t buffer[1024];
    size_t bytes_read = read(server_fd, buffer, sizeof(buffer));

    // LOG_INFO() << fmt::format("{}", std::vector<uint8_t>(buffer, buffer + bytes_read));
    for (size_t i = 0; i < bytes_read; ++i)
        std::cout << buffer[i] << ", ";

    return EXIT_SUCCESS;
}
