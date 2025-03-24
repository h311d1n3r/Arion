cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)

find_package(arion REQUIRED)

add_executable(disassembler disassembler.cpp)

target_link_libraries(disassembler PRIVATE arion::arion)