cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services C ASM)

add_subdirectory(process_manager)
add_subdirectory(vm_manager)
add_subdirectory(file_system)
add_subdirectory(drivers)
add_subdirectory(crypto)
add_subdirectory(console)
add_subdirectory(boot_displayd)

# Legacy monolithic network service (transitional)
add_subdirectory(net)

# Profile-driven network stack and manager
option(BHARAT_BUILD_NETWORK_STUBS "Build network manager and stack user-space stubs" ON)
if(BHARAT_BUILD_NETWORK_STUBS)
    add_subdirectory(netmgr)
    add_subdirectory(netstack)
endif()
