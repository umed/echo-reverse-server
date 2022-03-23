#include "logging/logging.hpp"

#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>

constexpr int PORT = 8080;

int main()
{
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOG_CRITICAL() << "Failed to create socket";
        return EXIT_FAILURE;
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        LOG_CRITICAL() << "Failed to set socket options";
        return EXIT_FAILURE;
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG_CRITICAL() << "Failed to bind socket to address";
        return EXIT_FAILURE;
    }
    
}
