cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

set(TARGET_NAME echo_reverse_server)

set(SOURCES
        main.cpp
        event_handlers/event_handlers.cpp
        events/epoller.cpp
        utils/fd_helpers.cpp
        net/tcp_server.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${TARGET_NAME} PRIVATE fmt::fmt CLI11 Threads::Threads spdlog)
