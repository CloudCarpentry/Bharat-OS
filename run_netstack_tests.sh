#!/bin/bash
gcc -I../services/netstack/src -Ilib/packet/include -Ilib/runtime/include \
    tests/test_netstack_phase2.c \
    lib/runtime/src/freestanding_string.c \
    services/netstack/src/netbuf.c \
    services/netstack/src/checksum.c \
    services/netstack/src/ethernet.c \
    services/netstack/src/arp.c \
    services/netstack/src/ipv4.c \
    services/netstack/src/icmp.c \
    services/netstack/src/udp.c \
    services/netstack/src/tcp.c \
    services/netstack/src/socket_table.c \
    services/netstack/src/loopback.c \
    services/netstack/src/driver_virtio_adapter.c \
    drivers/virtio_net/virtio_net.c \
    lib/packet/src/packet.c \
    -o tests/test_netstack_phase2
./tests/test_netstack_phase2
