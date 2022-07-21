#include "data.hpp"

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

constexpr int PORT = 9000;

void ReadResponse(int server_fd)
{
    uint8_t buffer[1024];
    size_t bytes_read;
    while ((bytes_read = read(server_fd, buffer, sizeof(buffer))) > 0) {
        LOG_INFO() << "read";
        LOG_INFO() << fmt::format("{}", std::vector<uint8_t>(buffer, buffer + bytes_read));
        std::cout << std::endl;
        LOG_INFO() << "end read/print";
    }
}

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

    // send(server_fd, BIG_ARRAY, sizeof(BIG_ARRAY), 0);
    // LOG_INFO() << "BIG_ARRAY message sent";
    // ReadResponse(server_fd);

    send(server_fd, SMALL_ARRAY, sizeof(SMALL_ARRAY), 0);
    LOG_INFO() << "SMALL_ARRAY message sent";
    ReadResponse(server_fd);

    close(server_fd);
    return EXIT_SUCCESS;
}
