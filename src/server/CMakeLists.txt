cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME echo_reverse_server)

# set(HEADERS
#         event_handlers
#         events
#         net
#         utils
# )

set(SOURCES
        main.cpp
        event_handlers/event_handlers.cpp
        events/epoller.cpp
        utils/file_descriptor_holder.cpp
        net/tcp_server.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${TARGET_NAME} logging fmt::fmt)
