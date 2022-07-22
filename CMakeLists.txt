cmake_minimum_required(VERSION 3.16)

project(echo_reverse_server)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    message(STATUS "CMAKE_BUILD_TYPE is not defined. ${CMAKE_BUILD_TYPE} will be used.")
endif()

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")

string (TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower_case)
if (cmake_build_type_lower_case STREQUAL "debug")
    message(STATUS "Debug mode: enable asan and other handy checks")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector -fno-omit-frame-pointer")
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_subdirectory(src)
add_subdirectory(vendor)
