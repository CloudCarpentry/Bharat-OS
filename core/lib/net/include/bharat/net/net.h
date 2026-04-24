#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @file net.h
 * @brief Common network definitions for Bharat-OS.
 *
 * Provides interface identifiers, address families, and link state
 * enums used across the network subsystem and services.
 */

// Interface identifier
typedef uint32_t net_if_id_t;

#define NET_IF_ID_INVALID 0

// Address families
typedef enum {
    NET_AF_UNSPEC = 0,
    NET_AF_INET = 1,   // IPv4
    NET_AF_INET6 = 2,  // IPv6
    NET_AF_PACKET = 3  // Raw packet access
} net_af_t;

// Link state
typedef enum {
    NET_LINK_DOWN = 0,
    NET_LINK_UP = 1,
    NET_LINK_UNKNOWN = 2
} net_link_state_t;

// Placeholder for an interface configuration structure
typedef struct {
    net_if_id_t id;
    net_link_state_t state;
    uint32_t mtu;
} net_if_config_t;

// Basic initialization contract (stub)
void libnet_init(void);
