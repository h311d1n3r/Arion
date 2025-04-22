cmake_minimum_required(VERSION 3.24)
project(Arion)
set(ARION_VERSION 1.0.2-alpha)

include(CMakePackageConfigHelpers)

option(DEV "Enable development mode. Bypasses compilation of dependencies." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -rdynamic -O0")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(UNICORN_ARCH "x86;arm;arm64" CACHE STRING "Build only x86, ARM, and ARM64 for Unicorn")

set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "Enable X86 support in Capstone")
set(CAPSTONE_ARM_SUPPORT ON CACHE BOOL "Enable ARM support in Capstone")
set(CAPSTONE_ARM64_SUPPORT ON CACHE BOOL "Enable ARM64 support in Capstone")

set(CAPSTONE_MIPS_SUPPORT OFF CACHE BOOL "Disable MIPS support in Capstone")
set(CAPSTONE_PPC_SUPPORT OFF CACHE BOOL "Disable PPC support in Capstone")
set(CAPSTONE_SPARC_SUPPORT OFF CACHE BOOL "Disable SPARC support in Capstone")
set(CAPSTONE_SYSZ_SUPPORT OFF CACHE BOOL "Disable SYSZ support in Capstone")
set(CAPSTONE_XCORE_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_M68K_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_SYSTEMZ_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_TMS320C64X_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_EVM_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_MOS65XX_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_WASM_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_BPF_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_RISCV_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_SH_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")
set(CAPSTONE_TRICORE_SUPPORT OFF CACHE BOOL "Disable TRICORE support in Capstone")
set(CAPSTONE_ALPHA_SUPPORT OFF CACHE BOOL "Disable ALPHA support in Capstone")
set(CAPSTONE_HPPA_SUPPORT OFF CACHE BOOL "Disable HPPA support in Capstone")
set(CAPSTONE_LOONGARCH_SUPPORT OFF CACHE BOOL "Disable LOONGARCH support in Capstone")
set(CAPSTONE_XTENSA_SUPPORT OFF CACHE BOOL "Disable XTENSA support in Capstone")
set(CAPSTONE_ARC_SUPPORT OFF CACHE BOOL "Disable ARC support in Capstone")
set(CAPSTONE_M680X_SUPPORT OFF CACHE BOOL "Disable XCore support in Capstone")

set(KEYSTONE_ARCH "X86;ARM;ARM64" CACHE STRING "Build only X86, ARM, and ARM64 for Keystone")
set(LIEF_PYTHON_API OFF CACHE BOOL "Disable Python bindings for LIEF")
set(LIEF_DOC OFF CACHE BOOL "Disable LIEF documentation")
set(LIEF_EXAMPLES OFF CACHE BOOL "Disable LIEF examples")
set(LIEF_TESTS OFF CACHE BOOL "Disable LIEF tests")
set(LIEF_ARCHITECTURES "x86;arm;arm64" CACHE STRING "Build only x86, ARM, and ARM64 for LIEF")

# Check for cargo installation
find_program(CARGO_EXECUTABLE cargo)
if (NOT CARGO_EXECUTABLE)
    message(FATAL_ERROR "cargo could not be found. Please install it to proceed.")
endif()

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

# UnicornAFL is added as part of source code with using add_subdirectory because its CMakeLists.txt would load embedded Unicorn
file(GLOB_RECURSE SOURCES "src/*.cpp" "lib/unicornafl/unicornafl.cpp")

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

set(ARION_LIB_LINK_MODE PRIVATE)

if(DEV)

set(ARION_LIB_LINK_MODE INTERFACE)

# Find libraries on the system
find_library(UNICORN_LIB unicorn REQUIRED)
find_library(KEYSTONE_LIB keystone REQUIRED)
find_library(CAPSTONE_LIB capstone REQUIRED)
find_library(LIEF_LIB LIEF REQUIRED)
# Try to load debug version of spdlog first
find_library(SPDLOG_LIB spdlogd)
if(NOT SPDLOG_LIB)
find_library(SPDLOG_LIB spdlog REQUIRED)
find_library(UDBSERVER_LIB udbserver REQUIRED)
endif()

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

set(UDBSERVER_PATH "${CMAKE_SOURCE_DIR}/lib/udbserver")
set(UDBSERVER_BUILD_PATH "${UDBSERVER_PATH}/build")
set(UDBSERVER_LIB_NAME "libudbserver.a")
set(UDBSERVER_LIB "${UDBSERVER_BUILD_PATH}/${UDBSERVER_LIB_NAME}")

add_custom_target(udbserver_build ALL
    DEPENDS ${UDBSERVER_LIB}
)

add_custom_command(
    COMMENT "Building the udbserver library"
    OUTPUT ${UDBSERVER_LIB}
    COMMAND mkdir -p ${UDBSERVER_BUILD_PATH}
    COMMAND cargo install cargo-c
    COMMAND cargo cinstall --release --prefix=/usr --destdir=${UDBSERVER_BUILD_PATH}
    COMMAND find "${UDBSERVER_BUILD_PATH}/usr" -name ${UDBSERVER_LIB_NAME} -exec cp {} ${UDBSERVER_LIB} \\\;
    WORKING_DIRECTORY ${UDBSERVER_PATH}
    DEPENDS "${UDBSERVER_PATH}/Cargo.toml"
)

endif()

target_link_libraries(arion ${ARION_LIB_LINK_MODE} ${UNICORN_LIB} ${CAPSTONE_LIB} ${KEYSTONE_LIB} ${LIEF_LIB} ${SPDLOG_LIB} ${UUID_LIB} ${UDBSERVER_LIB})

# This library should not bring version issues
find_path(UUID_INCLUDE_DIR uuid/uuid.h)

if(DEV)

find_path(UNICORN_INCLUDE_DIR unicorn/unicorn.h)
find_path(UNICORNAFL_INCLUDE_DIR unicornafl/unicornafl.h)
find_path(KEYSTONE_INCLUDE_DIR keystone/keystone.h)
find_path(CAPSTONE_INCLUDE_DIR capstone/capstone.h)
find_path(LIEF_INCLUDE_DIR LIEF/LIEF.hpp)
find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)
find_path(UDBSERVER_LIB udbserver.h)

else()

set(UNICORN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/unicorn/include)
set(UNICORNAFL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/unicornafl/include)
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
add_include_lib(${UNICORNAFL_INCLUDE_DIR} unicornafl)
add_include_lib(${CAPSTONE_INCLUDE_DIR} capstone)
add_include_lib(${KEYSTONE_INCLUDE_DIR} keystone)
add_include_lib(${LIEF_INCLUDE_DIR} LIEF)
add_include_lib(${SPDLOG_INCLUDE_DIR} spdlog)

# Needed because UnicornAFL is built with CMakeLists.txt
set(UNICORNAFL_BUILD_INCLUDE_DIRS $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/unicornafl/include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/unicornafl/include/unicornafl>)

target_include_directories(arion PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include> ${UNICORNAFL_BUILD_INCLUDE_DIRS} $<INSTALL_INTERFACE:include> $<INSTALL_INTERFACE:include/arion> ${UUID_INCLUDE_DIR})

install(TARGETS arion
        EXPORT arionTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        INCLUDES DESTINATION include)

# Install CMake package

install(
        EXPORT arionTargets
        FILE arionTargets.cmake
        NAMESPACE arion::
        DESTINATION lib/cmake/arion
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/arionConfigVersion.cmake"
        VERSION ${ARION_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arionConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/arionConfig.cmake"
        INSTALL_DESTINATION lib/cmake/arion
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/arionConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/arionConfigVersion.cmake"
        DESTINATION lib/cmake/arion
)

set(ARION_INCLUDE_INSTALL_DIR ${PROJECT_SOURCE_DIR}/include/arion)

install(DIRECTORY ${ARION_INCLUDE_INSTALL_DIR} DESTINATION include)

# Deploy headers of libraries

set(UNICORN_INCLUDE_INSTALL_DIR ${UNICORN_INCLUDE_DIR}/unicorn)
set(UNICORNAFL_INCLUDE_INSTALL_DIR ${UNICORNAFL_INCLUDE_DIR}/unicornafl)
set(CAPSTONE_INCLUDE_INSTALL_DIR ${CAPSTONE_INCLUDE_DIR}/capstone)
set(KEYSTONE_INCLUDE_INSTALL_DIR ${KEYSTONE_INCLUDE_DIR}/keystone)
set(LIEF_INCLUDE_INSTALL_DIR ${LIEF_INCLUDE_DIR}/LIEF)
set(LIEF_BUILD_INCLUDE_INSTALL_DIR ${CMAKE_BINARY_DIR}/lib/lief/include/LIEF)
set(SPDLOG_INCLUDE_INSTALL_DIR ${SPDLOG_INCLUDE_DIR}/spdlog)

install(DIRECTORY ${UNICORN_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${UNICORNAFL_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${CAPSTONE_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${KEYSTONE_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${LIEF_INCLUDE_INSTALL_DIR} DESTINATION include/arion)

if(NOT DEV)
install(DIRECTORY ${LIEF_BUILD_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
endif()

install(DIRECTORY ${SPDLOG_INCLUDE_INSTALL_DIR} DESTINATION include/arion)

# Build tools
add_subdirectory(tools)
