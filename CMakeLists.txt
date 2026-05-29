cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(cubesat_slave C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

# ===== Pico 1: I2C + UART =====
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
    hardware_uart
)

pico_enable_stdio_usb(cubesat_slave 1)
pico_enable_stdio_uart(cubesat_slave 0)
pico_add_extra_outputs(cubesat_slave)

# Skip the tud_cdc_connected() / DTR check inside stdio_usb_in_chars so that
# getchar_timeout_us() works even when the host (dashboard / jSerialComm) has
# not asserted DTR — which is the default behaviour of most serial libraries.
#
# Must be add_compile_definitions (global) so the flag reaches the pico-sdk's
# own stdio_usb.c library file, not just our own source files.
add_compile_definitions(PICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1)

# ===== Pico 2: SPI propulsor =====
add_executable(pico2_propulsor
    pico2.c
)

target_include_directories(pico2_propulsor PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico2_propulsor
    pico_stdlib
    hardware_spi
)

pico_enable_stdio_usb(pico2_propulsor 1)
pico_enable_stdio_uart(pico2_propulsor 0)
pico_add_extra_outputs(pico2_propulsor)