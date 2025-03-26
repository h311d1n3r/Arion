cmake_minimum_required(VERSION 3.10)
project(Example)

set(CMAKE_CXX_STANDARD 17)

find_package(arion REQUIRED)

add_executable(fork_handler fork_handler.cpp)
target_link_libraries(fork_handler PRIVATE arion::arion)

add_executable(target target.c)
