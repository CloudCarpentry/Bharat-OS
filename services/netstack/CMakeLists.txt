cmake_minimum_required(VERSION 3.20)
project(netstack C)

set(NETSTACK_SRC
    src/arp.c
    src/checksum.c
    src/driver_virtio_adapter.c
    src/ethernet.c
    src/icmp.c
    src/ipv4.c
    src/loopback.c
    src/main.c
    src/netbuf.c
    src/netstack_stubs.c
    src/socket_table.c
    src/tcp.c
    src/udp.c
)
add_executable(netstack ${NETSTACK_SRC})
target_link_libraries(netstack PRIVATE bharat_user_buildopts bharat_freestanding bharat_libpacket bharat_libcmin bharat_libruntime bharat_version_api)

bharat_register_component(
    TARGET netstack
    KIND "service"
    VERSION "${BHARAT_SVC_NETSTACK_VERSION}"
    IFACE_VERSION "${BHARAT_SVC_NETSTACK_IFACE}"
    CHANNEL "${BHARAT_SVC_NETSTACK_CHANNEL}"
)

target_include_directories(netstack PRIVATE ../../lib/packet/include ../../include)
target_compile_options(netstack PRIVATE -Wall -ffreestanding -nostdlib )
target_link_options(netstack PRIVATE -nostdlib)
# Link against necessary system primitives/stubs here in the future

bharat_configure_userspace_binary(netstack)
