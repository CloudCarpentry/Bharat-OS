cmake_minimum_required(VERSION 3.20)

add_library(bharatos_driver_baseline_led STATIC
    led_core.c
)

target_include_directories(bharatos_driver_baseline_led PUBLIC ../include)
target_include_directories(bharatos_driver_baseline_led PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)
target_link_libraries(bharatos_driver_baseline_led PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_generated_includes)
target_compile_options(bharatos_driver_baseline_led PRIVATE -ffreestanding -nostdlib -Wall)