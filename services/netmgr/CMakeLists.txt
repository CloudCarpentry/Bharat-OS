cmake_minimum_required(VERSION 3.20)
project(netmgr C)

set(NETMGR_SOURCES
    src/main.c
    src/interface_table.c
    src/address_table.c
    src/route_table.c
    src/neighbor_cache.c
    src/driver_policy.c
    src/driver_health.c
    src/capability_checks.c
    src/ipc_dispatch.c
)

add_executable(netmgr ${NETMGR_SOURCES})
    target_link_libraries(netmgr PRIVATE bharat_user_buildopts bharat_freestanding bharat_libruntime bharat_version_api)

bharat_register_component(
    TARGET netmgr
    KIND "service"
    VERSION "${BHARAT_SVC_NETMGR_VERSION}"
    IFACE_VERSION "${BHARAT_SVC_NETMGR_IFACE}"
    CHANNEL "${BHARAT_SVC_NETMGR_CHANNEL}"
)

target_include_directories(netmgr PRIVATE
    src/
    ../../services/core/subsysmgr/include
    ../../lib/net/include
    ../../include
)

target_compile_options(netmgr PRIVATE -Wall -ffreestanding -nostdlib -fPIE -fPIC)
target_link_options(netmgr PRIVATE -nostdlib -no-pie)
# Link against necessary system primitives/stubs here in the future

bharat_configure_userspace_binary(netmgr)

# Capability enforcement tests
# Only build this test when targeting the host (not cross-compiling)
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Skipping test_netmgr_caps build during cross-compilation")
else()
    add_executable(test_netmgr_caps tests/test_netmgr_caps.c src/capability_checks.c src/ipc_dispatch.c)
    target_include_directories(test_netmgr_caps PRIVATE src ../../include ../../lib/cap/include)
    target_link_libraries(test_netmgr_caps PRIVATE libpacket bharat_runtime_freestanding libnet)
    set_target_properties(test_netmgr_caps PROPERTIES COMPILE_OPTIONS "")
    set_target_properties(test_netmgr_caps PROPERTIES LINK_OPTIONS "")
    add_test(NAME test_netmgr_caps COMMAND test_netmgr_caps)
endif()
