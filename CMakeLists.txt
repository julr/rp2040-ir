cmake_minimum_required(VERSION 3.16)

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
    "In-source builds are not allowed!
    Please create a directory and run cmake from there, passing the path to this source directory as the last argument.
    This process created the file `CMakeCache.txt' and the directory `CMakeFiles', please delete them.")
endif()

include(pico_sdk_import.cmake)

project(irdecoder C CXX ASM)

pico_sdk_init()

add_compile_options(-Wall)

add_executable(${CMAKE_PROJECT_NAME})

target_link_libraries(${CMAKE_PROJECT_NAME}
    pico_stdlib
    pico_multicore
    pico_time
    pico_unique_id
    pico_bootsel_via_double_reset
    tinyusb_device
    hardware_pio
    etl
)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 0)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)

pico_add_extra_outputs(${CMAKE_PROJECT_NAME})

add_subdirectory(lib/etl)
add_subdirectory(src)