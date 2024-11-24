# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(led_strip C CXX ASM)

pico_sdk_init()

add_executable(led_strip led_strip.c apa102.c)

pico_generate_pio_header(led_strip ${CMAKE_CURRENT_LIST_DIR}/apa102.pio)
pico_generate_pio_header(led_strip ${CMAKE_CURRENT_LIST_DIR}/spi_rx.pio)

target_link_libraries(led_strip 
    pico_stdlib 
    hardware_spi 
    hardware_adc 
    hardware_pio
    hardware_dma 
    pico_stdio_usb 
    pico_rand)

pico_enable_stdio_usb(led_strip 1)

pico_add_extra_outputs(led_strip)

