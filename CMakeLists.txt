cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)



project(larrys_controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )


# compile src

add_executable(larrys_controller)

target_sources(larrys_controller PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/mcp3008.c

        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(larrys_controller PUBLIC
         ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
# include 
target_link_libraries(larrys_controller PUBLIC pico_stdlib  hardware_spi tinyusb_device tinyusb_board )

# Enable usb output, disable uart output
#pico_enable_stdio_usb(larrys_controller 1)
# pico_enable_stdio_uart(larrys_controller 0)

pico_add_extra_outputs(larrys_controller)