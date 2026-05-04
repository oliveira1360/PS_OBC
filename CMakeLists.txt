cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(cubesat_slave C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(cubesat_slave
    cubesat_slave.c
    i2c_multi.c
)

target_include_directories(cubesat_slave PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/I2C-slave-multi-address-RP2040/sdk
)

target_link_libraries(cubesat_slave
    pico_stdlib
    hardware_irq
    hardware_pio
    hardware_i2c
)

pico_enable_stdio_usb(cubesat_slave 1)
pico_enable_stdio_uart(cubesat_slave 0)

pico_add_extra_outputs(cubesat_slave)