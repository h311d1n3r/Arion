cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)

find_library(ARION_LIB NAMES arion)
find_path(ARION_INCLUDE_DIR arion/arion.hpp)

add_executable(speed_tester speed_tester.cpp)

target_link_libraries(speed_tester PRIVATE ${ARION_LIB})
target_link_libraries(cpuid_handler PRIVATE unicorn ${UNICORN_LIB})
target_include_directories(speed_tester PRIVATE ${ARION_INCLUDE_DIR}/arion)