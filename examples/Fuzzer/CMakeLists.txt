cmake_minimum_required(VERSION 3.10)
project(Example)

set(CMAKE_CXX_STANDARD 17)

find_package(arion REQUIRED)

add_executable(fuzzer fuzzer.cpp)
target_link_libraries(fuzzer PRIVATE arion::arion)

add_executable(target target.cpp)