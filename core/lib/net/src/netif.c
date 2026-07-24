#include "net/netif.h"
#include <string.h>

void netif_init(netif_t *ni, const char *name, const netif_ops_t *ops, void *priv) {
    if (!ni) return;
    memset(ni, 0, sizeof(netif_t));
    if (name) {
        strncpy(ni->name, name, NETIF_NAME_MAX - 1);
    }
    if (ops) {
        ni->ops = *ops;
    }
    ni->driver_priv = priv;
    ni->mtu = 1500; /* Default Ethernet MTU */
}

int netif_set_up(netif_t *ni) {
    if (!ni) return -1;
    if (ni->is_up) return 0;

    int ret = 0;
    if (ni->ops.up) {
        ret = ni->ops.up(ni);
    }
    if (ret == 0) {
        ni->is_up = true;
    }
    return ret;
}

int netif_set_down(netif_t *ni) {
    if (!ni) return -1;
    if (!ni->is_up) return 0;

    int ret = 0;
    if (ni->ops.down) {
        ret = ni->ops.down(ni);
    }
    if (ret == 0) {
        ni->is_up = false;
    }
    return ret;
}

void netif_set_ip(netif_t *ni, ipv4_addr_t ip, ipv4_addr_t mask, ipv4_addr_t gw) {
    if (!ni) return;
    ni->ip_addr = ip;
    ni->netmask = mask;
    ni->gateway = gw;
}

void netif_set_mac(netif_t *ni, const mac_addr_t *mac) {
    if (!ni || !mac) return;
    memcpy(&ni->mac, mac, sizeof(mac_addr_t));
}

int netif_tx(netif_t *ni, packet_t *pkt) {
    if (!ni || !pkt) return -1;
    if (!ni->is_up || !ni->link_up) {
        return -1; // Link down
    }
    if (ni->ops.tx) {
        return ni->ops.tx(ni, pkt);
    }
    return -1;
}
