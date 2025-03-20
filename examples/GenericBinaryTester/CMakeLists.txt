cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)

find_library(ARION_LIB NAMES arion)
find_path(ARION_INCLUDE_DIR arion/arion.hpp)

add_executable(gpt gpt.cpp)

target_link_libraries(gpt PRIVATE ${ARION_LIB})
target_include_directories(gpt PRIVATE ${ARION_INCLUDE_DIR}/arion)

add_subdirectory(binaries/sleep)