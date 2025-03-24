cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)

find_package(arion REQUIRED)

add_executable(speed_tester speed_tester.cpp)

target_link_libraries(speed_tester PRIVATE arion::arion)