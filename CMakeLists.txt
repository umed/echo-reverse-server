cmake_minimum_required(VERSION 3.16)

project(echo_reverse_server)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "CMAKE_BUILD_TYPE is not defined. ${CMAKE_BUILD_TYPE} will be used.")
endif()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)

enable_testing()

add_subdirectory(src)
add_subdirectory(vendor)
