cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services C ASM)

add_subdirectory(process_manager)
add_subdirectory(vm_manager)

# Phase 1: Native Service Skeletons
option(BHARAT_BUILD_USER_SERVICES_STUBS "Build foundational user-space service stubs" ON)
if(BHARAT_BUILD_USER_SERVICES_STUBS)
    add_subdirectory(init)
    add_subdirectory(namesvc)
endif()
add_subdirectory(file_system)
add_subdirectory(drivers)
add_subdirectory(crypto)
add_subdirectory(console)
add_subdirectory(boot_displayd)
add_subdirectory(net)
