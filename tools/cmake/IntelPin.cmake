cmake_minimum_required(VERSION 3.10)
project(IntelPin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -rdynamic -O0")

if(DEFINED ENV{PIN_ROOT})
    set(PIN_DIR $ENV{PIN_ROOT})

    add_library(IntelPin INTERFACE)

    target_include_directories(IntelPin INTERFACE
            ${PIN_DIR}/source/include/pin
            ${PIN_DIR}/source/include/pin/gen
            ${PIN_DIR}/extras/components/include
            ${PIN_DIR}/extras/xed-intel64/include/xed
            ${PIN_DIR}/source/tools/Utils
            ${PIN_DIR}/source/tools/InstLib
            )

    target_include_directories(IntelPin SYSTEM INTERFACE
            ${PIN_DIR}/extras/cxx/include
            ${PIN_DIR}/extras/crt/include
            ${PIN_DIR}/extras/crt/include/arch-x86_64
            ${PIN_DIR}/extras/crt/include/kernel/uapi
            ${PIN_DIR}/extras/crt/include/kernel/uapi/asm-x86
            )

    target_compile_definitions(IntelPin INTERFACE
            TARGET_IA32E
            HOST_IA32E
            TARGET_LINUX
            __PIN__=1
            PIN_CRT=1
            )

    target_compile_options(IntelPin INTERFACE
            -Wall
            -Werror
            -Wno-unknown-pragmas
            -Wno-dangling-pointer
            -funwind-tables
            -fno-stack-protector
            -fasynchronous-unwind-tables
            -fomit-frame-pointer
            -fno-strict-aliasing
            -fno-exceptions
            -fno-rtti
            -fabi-version=2
            -fPIC
            -faligned-new
            -O3
            )

    target_link_options(IntelPin INTERFACE
            -shared
            -Wl,--hash-style=sysv
            -Wl,-Bsymbolic
            -Wl,--version-script=${PIN_DIR}/source/include/pin/pintool.ver
            -fabi-version=2
            -nostdlib
            )

    target_link_libraries(IntelPin INTERFACE
            ${PIN_DIR}/intel64/runtime/pincrt/crtbeginS.o
            ${PIN_DIR}/intel64/runtime/pincrt/crtendS.o
            pin
            xed
            pindwarf
            dwarf
            dl-dynamic
            c++
            c++abi
            m-dynamic
            c-dynamic
            unwind-dynamic
            )

    target_link_directories(IntelPin INTERFACE
            ${PIN_DIR}/intel64/runtime/pincrt
            ${PIN_DIR}/intel64/lib
            ${PIN_DIR}/intel64/lib-ext
            ${PIN_DIR}/extras/xed-intel64/lib
            )
else()
    message(WARNING "PIN_ROOT environment variable is not set. Can't compile \"IntelPin\".")
endif()

    function(add_pintool target)
        if(DEFINED PIN_DIR)
            add_library(${target} SHARED ${ARGN})
            target_link_libraries(${target} PRIVATE IntelPin)
        else()
            message(WARNING "PIN_ROOT environment variable is not set. Can't compile \"${target}\".")
        endif()
    endfunction()
