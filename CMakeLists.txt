cmake_minimum_required(VERSION 3.13)

# cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..

include(/usr/share/pico_sdk/external/pico_sdk_import.cmake)

enable_language(C CXX)
project(hdmi C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
pico_sdk_init()

add_executable(hdmi
    main.cc
    pixel.cc
)

add_compile_definitions(PICO_USE_STACK_GUARDS=0)
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap.ld)

pico_generate_pio_header(hdmi ${CMAKE_CURRENT_LIST_DIR}/hdmi.pio)

target_link_libraries(hdmi
    stdc++
    supc++
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_i2c
    hardware_pio
)

pico_add_extra_outputs(hdmi)

pico_set_float_implementation(hdmi pico)
pico_set_double_implementation(hdmi pico)

pico_enable_stdio_usb(hdmi 1)
pico_enable_stdio_uart(hdmi 0)
