cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 8.1.1
)

FetchContent_MakeAvailable(fmt)
