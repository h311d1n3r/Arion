cmake_minimum_required(VERSION 3.10)
project(PinTracer)

include(../cmake/IntelPin.cmake)

if(DEFINED ENV{PIN_ROOT})
    add_pintool(pin_tracer
        ${PIN_DIR} src/pin_tracer.cpp
    )

    target_include_directories(pin_tracer PRIVATE ../../include ../../lib/lief/include/ ../../lib/spdlog/include/ ../../lib/unicorn/include/ ../../lib/capstone/include/ ../../lib/keystone/include/)
else()
    message(WARNING "PIN_ROOT environment variable is not set. Can't compile \"PinTracer\".")
endif()
