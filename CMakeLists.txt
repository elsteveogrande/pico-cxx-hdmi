cmake_minimum_required(VERSION 3.13...3.27)
enable_language(C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(pico-sdk/external/pico_sdk_import.cmake)

project(hdmi.elf)

pico_sdk_init()

add_executable(hdmi.elf
    main.cc
    pixel.cc
)

add_compile_definitions(PICO_USE_STACK_GUARDS=0)
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap.ld)

pico_generate_pio_header(hdmi.elf ${CMAKE_CURRENT_LIST_DIR}/hdmi.pio)

target_link_libraries(hdmi.elf
    stdc++
    supc++
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_pio
)

pico_add_extra_outputs(hdmi.elf)

pico_set_float_implementation(hdmi.elf pico)
pico_set_double_implementation(hdmi.elf pico)

pico_enable_stdio_usb(hdmi.elf 1)
pico_enable_stdio_uart(hdmi.elf 0)
