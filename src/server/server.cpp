#include "logging.hpp"

#include "tcp_server.hpp"

#include <algorithm>

constexpr uint16_t PORT = 9123;

constexpr int MAX_CONNECTION_NUMBER = 1024;

int main()
{
    using namespace echo_reverse_server;
    net::TcpServer server(PORT, MAX_CONNECTION_NUMBER);
    server.Start();

    auto client = server.Accept();
    std::array<uint8_t, 1024> buffer;
    size_t bytes_read = client.Read(buffer);

    std::reverse(buffer.begin(), buffer.begin() + bytes_read);

    client.Send(buffer, bytes_read);
    LOG_INFO() << "Success" << bytes_read;
    for (int i = 0; i < bytes_read; ++i)
        std::cout << std::hex << static_cast<unsigned>(buffer[i]) << ' ';
    std::cout << std::endl;
    LOG_INFO() << "Finish";
    return EXIT_SUCCESS;
}
