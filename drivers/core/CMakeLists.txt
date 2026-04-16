cmake_minimum_required(VERSION 3.20)

add_library(bharatos_driver_baseline_core STATIC
    device_registry.c
    driver_registry.c
    match.c
    event.c
    power.c
)

target_include_directories(bharatos_driver_baseline_core PUBLIC ../include)
target_include_directories(bharatos_driver_baseline_core PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bharatos_driver_baseline_core PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_generated_includes)
target_compile_options(bharatos_driver_baseline_core PRIVATE -ffreestanding -nostdlib -Wall)