cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME echo_reverse_client)

add_executable(${TARGET_NAME} client.cpp)

target_link_libraries(${TARGET_NAME}
    logging
    fmt::fmt
)
