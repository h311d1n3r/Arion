cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)

find_library(ARION_LIB NAMES arion)

add_executable(fork_handler fork_handler.cpp)

target_link_libraries(fork_handler PRIVATE ${ARION_LIB})

add_executable(target target.c)
