cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services C ASM)

add_subdirectory(process_manager)
add_subdirectory(vm_manager)

# Experimental / Scaffold Services
option(BHARAT_BUILD_EXPERIMENTAL_SERVICES "Build experimental and scaffold-only services" OFF)

# Phase 1: Native Service Skeletons
option(BHARAT_BUILD_USER_SERVICES_STUBS "Build foundational user-space service stubs" OFF)
if(BHARAT_BUILD_USER_SERVICES_STUBS AND BHARAT_BUILD_EXPERIMENTAL_SERVICES)
    add_subdirectory(init)
    add_subdirectory(namesvc)
    add_subdirectory(servicemgr)
endif()

# Core System Services (Multi-kernel / Distributed OS)
option(BHARAT_BUILD_CORE_SERVICES "Build core multi-kernel distributed OS services" OFF)
if(BHARAT_BUILD_CORE_SERVICES AND BHARAT_BUILD_EXPERIMENTAL_SERVICES)
    add_subdirectory(coremgr)
    add_subdirectory(memmgr)

    add_subdirectory(schedmgr)
    add_subdirectory(devmgr)
    add_subdirectory(storagemgr)
    add_subdirectory(faultmgr)
    add_subdirectory(telemetrymgr)
endif()
add_subdirectory(device)
add_subdirectory(system/filesystem)
add_subdirectory(system/console)
add_subdirectory(drivers)
add_subdirectory(security/crypto)
if(BHARAT_ENABLE_SERVICE_BOOT_DISPLAYD)
    add_subdirectory(system/boot_displayd)
endif()

if(NOT BHARAT_LEGACY_SUBSYS_LAYOUT)
    add_subdirectory(core/subsysmgr)
endif()

# Legacy monolithic network service (transitional/deprecated)
# TODO(PR3.1-TRANSITION): This is the legacy network path. netmgr and netstack are the forward path.
if(BHARAT_ENABLE_SERVICE_NETWORK)
    add_subdirectory(legacy/net)
endif()

# Profile-driven network stack and manager
option(BHARAT_BUILD_NETWORK_STUBS "Build network manager and stack user-space stubs" OFF)
if(BHARAT_BUILD_NETWORK_STUBS AND BHARAT_ENABLE_SERVICE_NETWORK)
    add_subdirectory(netmgr)
    add_subdirectory(netstack)
    if(BHARAT_BUILD_EXPERIMENTAL_SERVICES)
        add_subdirectory(netfast)
    endif()
endif()

if(BHARAT_BUILD_EXPERIMENTAL_SERVICES)
    add_subdirectory(power_mode)
    add_subdirectory(system/diag)
    add_subdirectory(sensor_hub)
    add_subdirectory(time_sync)
    add_subdirectory(can)
endif()
