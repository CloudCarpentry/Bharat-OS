cmake_minimum_required(VERSION 3.20)

add_library(bharatos_stack_can STATIC
    core/can_route.c
    core/can_filter.c
)
target_include_directories(bharatos_stack_can PUBLIC
    ${CMAKE_SOURCE_DIR}/stacks/include
    ${CMAKE_SOURCE_DIR}/services/include
    ${CMAKE_SOURCE_DIR}/drivers/include
    ${CMAKE_SOURCE_DIR}/lib/include
)
target_compile_options(bharatos_stack_can PRIVATE -ffreestanding -fno-builtin)
