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
    src/ipc_dispatch.c
    src/ipc_ops.c
    src/ipc_auth.c
    src/ipc_contract.c
    src/service_manifest.c
    ../../kernel/src/ipc/contract_validate.c
)

add_executable(netmgr ${NETMGR_SOURCES})
    target_link_libraries(netmgr PRIVATE bharat_user_buildopts bharat_freestanding bharat_libruntime bharat_version_api bharat_syscall)

bharat_register_component(
    TARGET netmgr
    KIND "service"
    VERSION "${BHARAT_SVC_NETMGR_VERSION}"
    IFACE_VERSION "${BHARAT_SVC_NETMGR_IFACE}"
    CHANNEL "${BHARAT_SVC_NETMGR_CHANNEL}"
)

target_include_directories(netmgr PRIVATE
    src/
    include/
    ../../services/core/subsysmgr/include
    ../../lib/net/include
    ../../include
    ../../kernel/include
    ../../lib/ipc/include
)

target_compile_options(netmgr PRIVATE -Wall -ffreestanding -nostdlib -fPIE -fPIC)
target_link_options(netmgr PRIVATE -nostdlib "LINKER:-no-pie")
# Link against necessary system primitives/stubs here in the future

bharat_configure_userspace_binary(netmgr)

# Capability enforcement tests
# Only build this test when targeting the host (not cross-compiling)
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Skipping test_netmgr_caps build during cross-compilation")
else()
    add_executable(test_netmgr_caps tests/test_netmgr_caps.c tests/test_netmgr_caps_stubs.c src/ipc_dispatch.c src/ipc_contract.c src/ipc_auth.c src/ipc_ops.c ../../kernel/src/ipc/contract_validate.c)
    target_include_directories(test_netmgr_caps PRIVATE src include ../../include ../../lib/cap/include ../../lib/net/include ../core/subsysmgr/include ../../lib/ipc/include ../../kernel/include ../../lib/runtime/include)
    target_link_libraries(test_netmgr_caps PRIVATE bharat_libpacket bharat_libruntime bharat_libnet bharat_libipc bharat_libcap_base)
    set_target_properties(test_netmgr_caps PROPERTIES COMPILE_OPTIONS "")
    set_target_properties(test_netmgr_caps PROPERTIES LINK_OPTIONS "")
    add_test(NAME test_netmgr_caps COMMAND test_netmgr_caps)
endif()
