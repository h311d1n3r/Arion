cmake_minimum_required(VERSION 3.10)
project(Test)

set(CMAKE_CXX_STANDARD 17)

find_package(arion REQUIRED)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE arion::arion)

add_executable(target target.cpp)