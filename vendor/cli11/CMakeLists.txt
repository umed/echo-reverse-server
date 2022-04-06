cmake_minimum_required(VERSION 3.16)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY httpom/CLIUtils/CLI11
  GIT_TAG        v2.2
)

FetchContent_MakeAvailable(cli11)
