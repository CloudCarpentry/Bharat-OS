#pragma once

#include <stdint.h>
#include "net/netif.h"

#define DNS_SERVER_PORT 53

typedef void (*dns_callback_t)(const char *name, ipv4_addr_t *ip, void *arg);

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qcount;
    uint16_t acount;
    uint16_t nscount;
    uint16_t arcount;
} dns_hdr_t;

int dns_client_init(ipv4_addr_t server_ip);
int dns_resolve(const char *name, dns_callback_t cb, void *arg);
void dns_client_tick(void);
