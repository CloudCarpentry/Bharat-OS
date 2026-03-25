#include "net/route.h"
#include <string.h>

#define MAX_ROUTES 16

static route_entry_t routing_table[MAX_ROUTES];

void route_init(void) {
    memset(routing_table, 0, sizeof(routing_table));
}

int route_add(ipv4_addr_t dest, ipv4_addr_t mask, ipv4_addr_t gateway, netif_t *ni, int metric) {
    for (int i=0; i<MAX_ROUTES; i++) {
        if (!routing_table[i].ni) {
            routing_table[i].dest = dest;
            routing_table[i].mask = mask;
            routing_table[i].gateway = gateway;
            routing_table[i].ni = ni;
            routing_table[i].metric = metric;
            return 0;
        }
    }
    return -1; /* Table full */
}

int route_del(ipv4_addr_t dest, ipv4_addr_t mask) {
    for (int i=0; i<MAX_ROUTES; i++) {
        if (routing_table[i].ni &&
            routing_table[i].dest.addr == dest.addr &&
            routing_table[i].mask.addr == mask.addr) {
            memset(&routing_table[i], 0, sizeof(route_entry_t));
            return 0;
        }
    }
    return -1;
}

netif_t* route_lookup(ipv4_addr_t dest, ipv4_addr_t *next_hop) {
    route_entry_t *best = NULL;
    uint32_t best_mask = 0;

    for (int i=0; i<MAX_ROUTES; i++) {
        route_entry_t *rt = &routing_table[i];
        if (!rt->ni) continue;

        if ((dest.addr & rt->mask.addr) == (rt->dest.addr & rt->mask.addr)) {
            if (!best || rt->mask.addr > best_mask ||
               (rt->mask.addr == best_mask && rt->metric < best->metric)) {
                best = rt;
                best_mask = rt->mask.addr;
            }
        }
    }

    if (best) {
        if (next_hop) {
            if (best->gateway.addr != 0) {
                *next_hop = best->gateway;
            } else {
                *next_hop = dest;
            }
        }
        return best->ni;
    }
    return NULL;
}
