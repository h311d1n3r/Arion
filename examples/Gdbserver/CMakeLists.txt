cmake_minimum_required(VERSION 3.10)
project(Example)

set(CMAKE_CXX_STANDARD 17)

find_package(arion REQUIRED)

add_executable(gdbserver gdbserver.cpp)

target_link_libraries(gdbserver PRIVATE arion::arion)