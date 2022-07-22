#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include "event_handlers/event_handlers.hpp"
#include "events/epoller.hpp"
#include "net/tcp_server.hpp"

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

#include "spdlog/spdlog.h"

#include <thread>

using namespace echo_reverse_server;

struct CliParams {
    CliParams()
        : thread_number(std::thread::hardware_concurrency())
        , max_connection_number(net::MAX_CONNECTION_NUMBER)
        , port(net::PORT)
        , max_event_number(events::MAX_EVENT_NUMBER)
    {
    }
    int thread_number;
    int max_connection_number;
    int max_event_number;
    int port;
};

CliParams ParseCliArgs(int argc, char** argv)
{
    CLI::App app { "Echo reverse server" };

    CliParams params;

    app.add_option("-p,--port", params.port, "Port to listen");
    app.add_option("-m,--connection-number", params.max_connection_number, "Max connection number");
    app.add_option("-t,--thread-number", params.thread_number, "Max connection number");
    app.add_option("-e,--event-number", params.max_event_number, "Max event number");

    app.parse(argc, argv);

    return params;
}

void SetupSpdlog()
{
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
#endif
    spdlog::set_pattern("[%D %T%z] [%^%l%$] [t %t] %v");
}

int main(int argc, char** argv)
{
    CliParams params;
    try {
        params = ParseCliArgs(argc, argv);
    } catch (const CLI::ParseError& e) {
        SPDLOG_CRITICAL("Failed to parse CLI arguments: ", e.what());
        return EXIT_FAILURE;
    }
    SetupSpdlog();
    net::TcpServer server(params.port, params.max_connection_number);
    SPDLOG_INFO("Starting server on port: {}", params.port);
    SPDLOG_INFO("Maximum of simultaneous connections is set to {}", params.max_connection_number);
    server.Start();

    SPDLOG_INFO("Setting up epoll events, max events number {}", params.max_event_number);
    events::Epoller epoller(server.Connection()->fd, params.max_event_number);

    auto waiter = [&epoller, &server](const epoll_event& event) {
        try {
            SPDLOG_INFO("Event received");
            if (event_handlers::ShouldCloseConnection(event)) {
                SPDLOG_INFO("Handle disconnect");
                event_handlers::HandleDisconnect(epoller, server, event);
            } else if (static_cast<event_handlers::Consumer*>(event.data.ptr)->client.fd == server.Connection()->fd) {
                SPDLOG_INFO("Handle connect");
                event_handlers::HandleConnect(epoller, server, event);
            } else {
                event_handlers::HandleClientEvent(epoller, server, event);
            }
        } catch (const KernelError& e) {
            SPDLOG_ERROR(e.what());
        } catch (const net::ConnectionLost& e) {
            SPDLOG_WARN("{}. Consumer will be dropped", e.what());
            if (event.data.fd != server.Connection()->fd) {
                event_handlers::HandleDisconnect(epoller, server, event);
            }
        } catch (const std::exception& e) {
            SPDLOG_ERROR(e.what());
        }
    };

    int num_threads_available = std::thread::hardware_concurrency();
    SPDLOG_INFO("Number of available threads: {}, will be used: {}", num_threads_available, params.thread_number);

    std::vector<std::thread> threads;
    for (int i = 0; i < params.thread_number; ++i) {
        threads.emplace_back([&] {
            epoller.Wait(waiter);
        });
    }

    SPDLOG_INFO("Threads are ready to process requests: {}", params.thread_number);

    SPDLOG_INFO("Starting handling requests by main thread as well");
    epoller.Wait(waiter);

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    SPDLOG_INFO("All threads finished, shutting down");
    return EXIT_SUCCESS;
}
