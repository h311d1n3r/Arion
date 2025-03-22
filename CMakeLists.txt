cmake_minimum_required(VERSION 3.24)
project(Arion)

option(DEV "Enable development mode. Bypasses compilation of dependencies." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -rdynamic -O0")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

file(GLOB_RECURSE SOURCES "src/*.cpp")

# Copy vdso.bin to build directory to prevent absolute path inclusion
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/vdso.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/res/vdso.bin ${CMAKE_BINARY_DIR}/vdso.bin
    DEPENDS ${CMAKE_SOURCE_DIR}/res/vdso.bin
)
# Embed vdso.bin inside ELF
add_custom_command(
    OUTPUT vdso.o
    COMMAND ld -r -b binary -o vdso.o vdso.bin
    DEPENDS vdso.bin
)

add_library(arion SHARED ${SOURCES} vdso.o)

target_compile_definitions(arion PRIVATE ARION_ONLY) # Useful flag to prevent using some header parts when compiling a module against Arion
set_property(TARGET arion PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET arion PROPERTY OUTPUT_NAME "arion")

# Prefer shared libraries when linking in development mode to reduce ELF size
if(DEV)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
endif()

# This library should not bring version issues
find_library(UUID_LIB uuid REQUIRED)

if(DEV)

# Find libraries on the system
find_library(UNICORN_LIB unicorn REQUIRED)
find_library(KEYSTONE_LIB keystone REQUIRED)
find_library(CAPSTONE_LIB capstone REQUIRED)
find_library(LIEF_LIB LIEF REQUIRED)
find_library(SPDLOG_LIB spdlogd REQUIRED)

else()

# Embed exact libraries with required versions
add_subdirectory(lib/unicorn EXCLUDE_FROM_ALL)
add_subdirectory(lib/capstone EXCLUDE_FROM_ALL)
add_subdirectory(lib/keystone EXCLUDE_FROM_ALL)
add_subdirectory(lib/lief EXCLUDE_FROM_ALL)
add_subdirectory(lib/spdlog EXCLUDE_FROM_ALL)

set(UNICORN_LIB unicorn)
set(CAPSTONE_LIB capstone)
set(KEYSTONE_LIB keystone)
set(LIEF_LIB LIB_LIEF)
set(SPDLOG_LIB spdlog)

set_target_properties(${UNICORN_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${CAPSTONE_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${KEYSTONE_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${LIEF_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${SPDLOG_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)

endif()

target_link_libraries(arion PUBLIC ${UNICORN_LIB} ${CAPSTONE_LIB} ${KEYSTONE_LIB} ${LIEF_LIB} ${SPDLOG_LIB} ${UUID_LIB})

# This library should not bring version issues
find_path(UUID_INCLUDE_DIR uuid/uuid.h)

if(DEV)

find_path(UNICORN_INCLUDE_DIR unicorn/unicorn.h)
find_path(KEYSTONE_INCLUDE_DIR keystone/keystone.h)
find_path(CAPSTONE_INCLUDE_DIR capstone/capstone.h)
find_path(LIEF_INCLUDE_DIR LIEF/LIEF.hpp)
find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)

else()

set(UNICORN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/unicorn/include)
set(CAPSTONE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/capstone/include)
set(KEYSTONE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/keystone/include)
set(LIEF_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/lief/include)
set(SPDLOG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/spdlog/include)

endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/arion)

function(add_include_lib lib_path lib_name)
    add_custom_target(
            ${lib_name}_symlink ALL
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${lib_path}/${lib_name}
            ${CMAKE_BINARY_DIR}/include/arion/${lib_name}
    )
    add_dependencies(arion ${lib_name}_symlink)
endfunction()

add_include_lib(${UNICORN_INCLUDE_DIR} unicorn)
add_include_lib(${CAPSTONE_INCLUDE_DIR} capstone)
add_include_lib(${KEYSTONE_INCLUDE_DIR} keystone)
add_include_lib(${LIEF_INCLUDE_DIR} LIEF)
add_include_lib(${SPDLOG_INCLUDE_DIR} spdlog)

target_include_directories(arion PUBLIC ${PROJECT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include ${UUID_INCLUDE_DIR})

install(TARGETS arion
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include)

set(ARION_INCLUDE_INSTALL_DIR ${PROJECT_SOURCE_DIR}/include/arion)

install(DIRECTORY ${ARION_INCLUDE_INSTALL_DIR} DESTINATION include)

# Deploy headers of libraries

set(UNICORN_INCLUDE_INSTALL_DIR ${UNICORN_INCLUDE_DIR}/unicorn)
set(CAPSTONE_INCLUDE_INSTALL_DIR ${CAPSTONE_INCLUDE_DIR}/capstone)
set(KEYSTONE_INCLUDE_INSTALL_DIR ${KEYSTONE_INCLUDE_DIR}/keystone)
set(LIEF_INCLUDE_INSTALL_DIR ${LIEF_INCLUDE_DIR}/LIEF)
set(LIEF_BUILD_INCLUDE_INSTALL_DIR ${CMAKE_BINARY_DIR}/lib/lief/include/LIEF)
set(SPDLOG_INCLUDE_INSTALL_DIR ${SPDLOG_INCLUDE_DIR}/spdlog)

install(DIRECTORY ${UNICORN_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${CAPSTONE_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${KEYSTONE_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${LIEF_INCLUDE_INSTALL_DIR} DESTINATION include/arion)

if(NOT DEV)
install(DIRECTORY ${LIEF_BUILD_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
endif()

install(DIRECTORY ${SPDLOG_INCLUDE_INSTALL_DIR} DESTINATION include/arion)

# Build tools
add_subdirectory(tools)
