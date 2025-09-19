cmake_minimum_required(VERSION 3.24)
project(Arion)
set(ARION_VERSION 1.0.1-alpha)

include(CMakePackageConfigHelpers)

option(TEST "Generate test targets" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -rdynamic -O0")
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif()

set(ARION_SUPPORTED_ARCHS x86 x86-64 arm arm64)
set(ARION_SUPPORTED_GNU_TRIPLETS i686-linux-gnu x86_64-linux-gnu arm-linux-gnueabi aarch64-linux-gnu)

set(UNICORN_ARCH "x86;arm;aarch64" CACHE STRING "Build only x86, ARM, and ARM64 for Unicorn")

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

if (TEST)
    foreach(ARCH IN LISTS ARION_SUPPORTED_ARCHS)
        set(ROOTFS_PATH "${CMAKE_SOURCE_DIR}/rootfs/${ARCH}")
        if(NOT IS_DIRECTORY "${ROOTFS_PATH}")
            message(FATAL_ERROR "[TEST] Missing rootfs for architecture '${ARCH}'.\nYou can either download it (faster and more stable) or build it with appropriate scripts.")
        endif()
    endforeach()
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

# This library should not bring version issues
find_library(UUID_LIB uuid REQUIRED)

# Main goal of this function is to prevent building a submodule everytime Arion is built (config.h files messing up)
function(add_arion_dependency lib_var mod_name target_name invalidate)
    set(mod_path "${CMAKE_SOURCE_DIR}/lib/${mod_name}")
    if(NOT EXISTS "${mod_path}")
        message(FATAL_ERROR "Module not found: ${mod_path}")
    endif()

    file(TIMESTAMP "${mod_path}" new_timestamp)

    set(mod_updated ${invalidate})

    set(timestamp_dir "${CMAKE_BINARY_DIR}/arion_times")
    set(timestamp_file "${CMAKE_BINARY_DIR}/arion_times/${mod_name}.txt")
    file(MAKE_DIRECTORY ${timestamp_dir})
    if(EXISTS "${timestamp_file}")
        file(READ "${timestamp_file}" old_timestamp)
        string(STRIP "${old_timestamp}" old_timestamp)
        if(new_timestamp GREATER old_timestamp)
            set(mod_updated TRUE)
        endif()
    else()
        set(mod_updated TRUE)
    endif()

    if(NOT mod_updated)
        file(GLOB_RECURSE found_lib_obj "${CMAKE_BINARY_DIR}/lib/${mod_name}/*.o")
        if(found_lib_obj)
            add_custom_target(${target_name})
            set(${lib_var} ${found_lib_obj} PARENT_SCOPE)
        else()
            set(mod_updated TRUE)
        endif()
    endif()

    if(mod_updated)
        add_subdirectory(${mod_path} EXCLUDE_FROM_ALL)
        file(WRITE "${timestamp_file}" "${new_timestamp}")
        set(${lib_var} ${target_name} PARENT_SCOPE)
        set_target_properties(${target_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
endfunction()

# Embed exact libraries with required versions
add_arion_dependency(UNICORN_LIB unicorn unicorn FALSE)
add_arion_dependency(CAPSTONE_LIB capstone capstone FALSE)
add_arion_dependency(KEYSTONE_LIB keystone keystone FALSE)
# Due to spdlog embedding by LIEF and because they have good caching we invalidate these two libraries
add_arion_dependency(LIEF_LIB lief LIB_LIEF TRUE)
add_arion_dependency(SPDLOG_LIB spdlog spdlog TRUE)

set(UNICORN_LIB_DIR ${CMAKE_BINARY_DIR}/lib/unicorn)

set(UDBSERVER_PATH "${CMAKE_SOURCE_DIR}/lib/udbserver")
set(UDBSERVER_CARGO_PATH "${UDBSERVER_PATH}/Cargo.toml")
set(UDBSERVER_TARGET_PATH "${CMAKE_BINARY_DIR}/lib/udbserver/target")
set(UDBSERVER_BUILD_PATH "${CMAKE_BINARY_DIR}/lib/udbserver/build")
set(UDBSERVER_LIB_NAME "libudbserver.a")
set(UDBSERVER_LIB "${UDBSERVER_BUILD_PATH}/${UDBSERVER_LIB_NAME}")

add_custom_command(
    OUTPUT ${UDBSERVER_LIB}
    COMMENT "Building the udbserver library"
    COMMAND mkdir -p ${UDBSERVER_BUILD_PATH}
    COMMAND cargo install cargo-c
    # Force unicorn-engine v2.1.3 crate and force rustup 1.88 to prevent lifetime errors
    COMMAND /bin/sh -c "RUSTFLAGS='-L native=${UNICORN_LIB_DIR} -l static=unicorn' CARGO_TARGET_DIR=${UDBSERVER_TARGET_PATH} \
        rustup install 1.88.0 && \
        rustup override set 1.88.0 && \
        cargo update -p unicorn-engine --precise 2.1.3 && \
        cargo cinstall --release --prefix=/usr --destdir=${UDBSERVER_BUILD_PATH}"
    COMMAND find "${UDBSERVER_BUILD_PATH}/usr" -name ${UDBSERVER_LIB_NAME} -exec cp {} ${UDBSERVER_LIB} \;
    WORKING_DIRECTORY ${UDBSERVER_PATH}
    DEPENDS ${UDBSERVER_CARGO_PATH}
    DEPENDS unicorn
    VERBATIM
)

add_custom_target(
    udbserver_build
    DEPENDS ${UDBSERVER_LIB}
)
    
add_dependencies(arion udbserver_build)

target_link_libraries(arion PRIVATE -Wl,--allow-multiple-definition ${UNICORN_LIB} ${CAPSTONE_LIB} ${KEYSTONE_LIB} ${LIEF_LIB} ${SPDLOG_LIB} ${UUID_LIB} ${UDBSERVER_LIB})

# This library should not bring version issues
find_path(UUID_INCLUDE_DIR uuid/uuid.h)

set(UNICORN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/unicorn/include)
set(UNICORNAFL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/unicornafl/include)
set(CAPSTONE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/capstone/include)
set(KEYSTONE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/keystone/include)
set(LIEF_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/lief/include)
set(SPDLOG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/spdlog/include)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/arion)

function(add_include_lib lib_path lib_name)
    set(symlink_path ${CMAKE_BINARY_DIR}/include/arion/${lib_name})
    
    add_custom_command(
        OUTPUT ${symlink_path}
        COMMENT "Creating symlink for ${lib_name}"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${lib_path}/${lib_name}
        ${symlink_path}
        DEPENDS ${lib_path}/${lib_name}
        VERBATIM
    )
    
    add_custom_target(
        create_${lib_name}_symlink
        DEPENDS ${symlink_path}
    )
    
    add_dependencies(arion create_${lib_name}_symlink)
endfunction()

add_include_lib(${UNICORN_INCLUDE_DIR} unicorn)
add_include_lib(${UNICORNAFL_INCLUDE_DIR} unicornafl)
add_include_lib(${CAPSTONE_INCLUDE_DIR} capstone)
add_include_lib(${KEYSTONE_INCLUDE_DIR} keystone)
add_include_lib(${LIEF_INCLUDE_DIR} LIEF)
add_include_lib(${SPDLOG_INCLUDE_DIR} spdlog)

# Needed because UnicornAFL is built with CMakeLists.txt
set(UNICORNAFL_BUILD_INCLUDE_DIRS $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/unicornafl/include> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib/unicornafl/include/unicornafl>)
# Needed to get config.h after LIEF was built a first time
set(LIEF_POST_BUILD_INCLUDE_DIR $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/lib/lief/include>)

target_include_directories(arion PUBLIC ${LIEF_POST_BUILD_INCLUDE_DIR} $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include> $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/arion> ${UNICORNAFL_BUILD_INCLUDE_DIRS} $<INSTALL_INTERFACE:include> $<INSTALL_INTERFACE:include/arion> ${UUID_INCLUDE_DIR})

if(TEST)
    # Tests with GoogleTest
    add_subdirectory(${CMAKE_SOURCE_DIR}/lib/googletest)
    enable_testing()
    file(GLOB TEST_TARGETS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/targets/*)

    list(LENGTH ARION_SUPPORTED_ARCHS archs_len)
    math(EXPR archs_last_i "${archs_len} - 1")
    foreach(i RANGE 0 ${archs_last_i})
        list(GET ARION_SUPPORTED_ARCHS ${i} arch)
        list(GET ARION_SUPPORTED_GNU_TRIPLETS ${i} gnu_triplet)
        set(arch_toolchains_bin ${CMAKE_CURRENT_SOURCE_DIR}/rootfs/${arch}/toolchains/bin)

        foreach(target_dir ${TEST_TARGETS})
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${target_dir}/CMakeLists.txt")
                get_filename_component(target_name ${target_dir} NAME)
                ExternalProject_Add(${target_name}_${arch}
                    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${target_dir}
                    BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${target_dir}/build_${arch}
                    CMAKE_ARGS
                        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                        -DCMAKE_C_COMPILER=${arch_toolchains_bin}/gcc
                        -DCMAKE_CXX_COMPILER=${arch_toolchains_bin}/g++
                    INSTALL_COMMAND ""
                )
            endif()
        endforeach()
    endforeach()

    file(GLOB_RECURSE TEST_SOURCES "tests/src/*.cpp")
    add_executable(
      arion_test
      ${TEST_SOURCES}
    )
    target_include_directories(
        arion_test
        PUBLIC
        include
        tests/include
    )
    target_link_libraries(
      arion_test
      arion
      GTest::gtest_main
    )
    target_compile_definitions(
        arion_test PRIVATE
        ARION_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    include(GoogleTest)
    gtest_discover_tests(arion_test)
endif()

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
install(DIRECTORY ${LIEF_BUILD_INCLUDE_INSTALL_DIR} DESTINATION include/arion)
install(DIRECTORY ${SPDLOG_INCLUDE_INSTALL_DIR} DESTINATION include/arion)

# Build tools
add_subdirectory(tools)
