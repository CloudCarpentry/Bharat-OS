#pragma once

#include "net/netif.h"
#include <stdint.h>

typedef struct {
    ipv4_addr_t dest;
    ipv4_addr_t mask;
    ipv4_addr_t gateway;
    netif_t *ni;
    int metric;
} route_entry_t;

int route_add(ipv4_addr_t dest, ipv4_addr_t mask, ipv4_addr_t gateway, netif_t *ni, int metric);
int route_del(ipv4_addr_t dest, ipv4_addr_t mask);
netif_t* route_lookup(ipv4_addr_t dest, ipv4_addr_t *next_hop);
void route_init(void);
