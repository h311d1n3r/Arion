cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)

find_library(ARION_LIB NAMES arion)
find_path(ARION_INCLUDE_DIR arion/arion.hpp)

add_executable(fork_handler fork_handler.cpp)

target_link_libraries(fork_handler PRIVATE ${ARION_LIB})
target_include_directories(fork_handler PRIVATE ${ARION_INCLUDE_DIR}/arion)

add_executable(target target.c)
