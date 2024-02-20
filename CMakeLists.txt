cmake_minimum_required(VERSION 3.10)

# For Clang to do parsing
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(System2)

set(CMAKE_C_STANDARD 99)

add_library(System2 "${CMAKE_CURRENT_LIST_DIR}/System2.c")
target_include_directories(System2 PUBLIC "${CMAKE_CURRENT_LIST_DIR}")