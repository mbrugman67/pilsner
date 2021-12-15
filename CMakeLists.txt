cmake_minimum_required(VERSION 3.12)
 
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# Include the .cmake file to get the SDK
include(pico_sdk_import.cmake)

# Project declaration
project(pilnser C CXX ASM)
 
file(GLOB SOURCE_FILES
     "*.h"
     "*.cpp"
     "./ipc/*.h"
     "./ipc/*.cpp"
     "./sys/*.h"
     "./sys/*.cpp"
     "./utils/*.h"
     "./utils/*.cpp"
     "./pilznet/*.h"
     "./pilznet/*.cpp"
     "./alibs/*.h"
     "./alibs/*.cpp"
     "./af/utility/*.h"
     "./af/utility/*.cpp"
     "./af/*.h"
     "./af/*.cpp"
)
# Initalise the SDK
pico_sdk_init()

add_executable(pilsner ${SOURCE_FILES})

# Libraries
target_link_libraries(pilsner pico_stdlib hardware_spi pico_time pico_sync pico_multicore hardware_rtc)
 
# Enable USB, UART output
pico_enable_stdio_usb(pilsner 1)
pico_enable_stdio_uart(pilsner 0)
 
# Enable extra outputs
pico_add_extra_outputs(pilsner)
