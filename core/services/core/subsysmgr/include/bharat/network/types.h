#ifndef BHARAT_NETWORK_TYPES_H
#define BHARAT_NETWORK_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t bnet_interface_id_t;
typedef uint16_t bnet_queue_id_t;

typedef enum {
    BNET_MAC_TYPE_ETHERNET = 1,
    BNET_MAC_TYPE_WIFI,
    BNET_MAC_TYPE_VIRTUAL
} bnet_mac_type_t;

typedef enum {
    BNET_IP_FAMILY_V4 = 1,
    BNET_IP_FAMILY_V6
} bnet_ip_family_t;

typedef enum {
    BNET_LINK_STATE_DOWN = 0,
    BNET_LINK_STATE_UP,
    BNET_LINK_STATE_UNKNOWN
} bnet_link_state_t;

typedef struct {
    uint8_t addr[6];
} bnet_mac_addr_t;

typedef struct {
    bnet_ip_family_t family;
    union {
        uint32_t v4;
        uint8_t v6[16];
    } addr;
} bnet_ip_addr_t;

typedef struct {
    bnet_ip_addr_t prefix;
    uint8_t length;
} bnet_ip_prefix_t;

typedef struct {
    bnet_ip_prefix_t dest;
    bnet_ip_addr_t gateway;
    bnet_interface_id_t iface_id;
    uint32_t metric;
} bnet_route_key_t;

typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint64_t rx_dropped;
    uint64_t tx_dropped;
} bnet_stats_t;

typedef struct {
    uint32_t flags;
    uint16_t length;
    uint16_t offload;
} bnet_packet_meta_t;

#endif // BHARAT_NETWORK_TYPES_H
