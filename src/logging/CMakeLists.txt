cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME logging)

add_library(${TARGET_NAME} INTERFACE)

target_include_directories(${TARGET_NAME} INTERFACE
${CMAKE_CURRENT_SOURCE_DIR}/)
