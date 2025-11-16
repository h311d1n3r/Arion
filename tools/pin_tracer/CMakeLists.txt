cmake_minimum_required(VERSION 3.10)
project(PinTracer)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0")
    include(../cmake/IntelPin.cmake)

    if(DEFINED ENV{PIN_ROOT})
        add_pintool(pin_tracer
            ${PIN_DIR} src/pin_tracer.cpp
        )

        target_include_directories(pin_tracer PRIVATE ${PROJECT_SOURCE_DIR}/../../include ${CMAKE_BINARY_DIR}/include)
    else()
        message(WARNING "PIN_ROOT environment variable is not set. Can't compile \"PinTracer\".")
    endif()
else()
    message(WARNING "Unsupported G++ version for PIN tools : ${CMAKE_CXX_COMPILER_VERSION}. Please use G++ < 15.")
endif()
