#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "packet/packet.h"

#define NETIF_NAME_MAX 16
#define MAC_ADDR_LEN 6

typedef struct {
    uint8_t addr[MAC_ADDR_LEN];
} mac_addr_t;

typedef struct {
    uint32_t addr; /* IPv4 address in network byte order */
} ipv4_addr_t;

typedef struct netif netif_t;

/* Interface hardware operations (provided by driver/glue) */
typedef struct {
    int (*tx)(netif_t *ni, packet_t *pkt);
    int (*up)(netif_t *ni);
    int (*down)(netif_t *ni);
} netif_ops_t;

struct netif {
    char name[NETIF_NAME_MAX];
    mac_addr_t mac;
    ipv4_addr_t ip_addr;
    ipv4_addr_t netmask;
    ipv4_addr_t gateway;
    bool is_up;
    bool link_up;
    uint32_t mtu;
    netif_ops_t ops;
    void *driver_priv; /* Driver private context */
};

void netif_init(netif_t *ni, const char *name, const netif_ops_t *ops, void *priv);
int netif_set_up(netif_t *ni);
int netif_set_down(netif_t *ni);
void netif_set_ip(netif_t *ni, ipv4_addr_t ip, ipv4_addr_t mask, ipv4_addr_t gw);
void netif_set_mac(netif_t *ni, const mac_addr_t *mac);
int netif_tx(netif_t *ni, packet_t *pkt);
