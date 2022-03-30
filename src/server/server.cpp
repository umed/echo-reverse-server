#include "logging.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

constexpr int PORT = 9123;

constexpr int MAX_CONNECTION_NUMBER = 1024;

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        LOG_CRITICAL() << "Failed to create socket";
        return EXIT_FAILURE;
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        LOG_CRITICAL() << "Failed to set socket options";
        return EXIT_FAILURE;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    uint32_t addrlen = sizeof(address);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), addrlen) < 0) {
        LOG_CRITICAL() << "Failed to bind socket to address";
        return EXIT_FAILURE;
    }
    if (listen(server_fd, MAX_CONNECTION_NUMBER) < 0) {
        LOG_CRITICAL() << "Failed to listen for connections";
        return EXIT_FAILURE;
    }
    LOG_INFO() << "Awaiting client";
    int client_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&address), &addrlen);
    if (client_socket < 0) {
        LOG_CRITICAL() << "Failed to accept new connection";
        return EXIT_FAILURE;
    }
    std::array<int8_t, 1024> buffer;
    size_t bytes_read;
    LOG_INFO() << "Reading bytes";
    bytes_read = read(client_socket, buffer.data(), buffer.size());
    // while (() > 0) {
    //     result_data.insert(result_data.end(), buffer.begin(), buffer.begin() + bytes_read);
    //     if (buffer[bytes_read - 1] == 0) {
    //         break;
    //     }
    // }
    std::vector<int8_t> result_data;
    std::reverse_copy(buffer.begin(), buffer.begin() + bytes_read, std::back_inserter(result_data));
    send(client_socket, result_data.data(), result_data.size(), 0);
    LOG_INFO() << "Success" << result_data.size();
    for (int i = 0; i < bytes_read; ++i)
        std::cout << buffer[i] << ' ';
    std::cout << std::endl;
    LOG_INFO() << "Finish";
    close(server_fd);
    return EXIT_SUCCESS;
}
