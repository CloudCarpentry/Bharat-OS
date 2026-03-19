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
    target_link_libraries(netmgr PRIVATE bharat_user_buildopts bharat_freestanding)

target_include_directories(netmgr PRIVATE
    src/
    ../../subsys/include
    ../../lib/net/include
)

target_compile_options(netmgr PRIVATE -Wall -ffreestanding -nostdlib -fPIE -fPIC)
target_link_options(netmgr PRIVATE -nostdlib -no-pie)
# Link against necessary system primitives/stubs here in the future
