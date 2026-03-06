cmake_minimum_required(VERSION 3.20)
project(BharatOS_Subsys C ASM)

include_directories(include)

# Subsystem manager daemon/library
set(SUBSYS_SOURCES
    src/manager.c
)

add_library(subsys_manager STATIC ${SUBSYS_SOURCES})
target_compile_options(subsys_manager PRIVATE -ffreestanding -nostdlib -Wall)
