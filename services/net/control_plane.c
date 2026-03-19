#include "control_plane.h"
//#include <string.h>
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

/*
 * Control Plane Implementation
 */

#define MAX_INTERFACES 16

static net_if_t g_interfaces[MAX_INTERFACES];
static uint32_t g_num_interfaces = 0;

void net_control_plane_init(void) {
    g_num_interfaces = 0;
    for (int i = 0; i < MAX_INTERFACES; i++) {
        g_interfaces[i].if_id = (uint32_t)-1; /* Invalid */
        g_interfaces[i].link_state = NET_LINK_DOWN;
        memset(&g_interfaces[i].stats, 0, sizeof(netdev_stats_t));
    }
}

int net_register_interface(const char* name, const uint8_t* mac, uint32_t mtu, uint32_t* out_if_id) {
    if (g_num_interfaces >= MAX_INTERFACES) {
        return -1;
    }

    uint32_t new_id = g_num_interfaces;
    net_if_t* iface = &g_interfaces[new_id];

    iface->if_id = new_id;

    /* Copy name safely */
    int i = 0;
    while (name && name[i] != '\0' && i < MAX_IF_NAME_LEN - 1) {
        iface->name[i] = name[i];
        i++;
    }
    iface->name[i] = '\0';

    /* Copy MAC */
    if (mac) {
        for (int j = 0; j < MAC_ADDR_LEN; j++) {
            iface->mac[j] = mac[j];
        }
    } else {
        for (int j = 0; j < MAC_ADDR_LEN; j++) {
            iface->mac[j] = 0;
        }
    }

    iface->mtu = mtu;
    iface->link_state = NET_LINK_DOWN; /* Default to down */

    memset(&iface->stats, 0, sizeof(netdev_stats_t));

    g_num_interfaces++;

    if (out_if_id) {
        *out_if_id = new_id;
    }

    return 0;
}

int net_set_link_state(uint32_t if_id, net_link_state_t state) {
    if (if_id >= g_num_interfaces) {
        return -1; /* Not found */
    }

    g_interfaces[if_id].link_state = state;
    return 0;
}

int net_get_stats(uint32_t if_id, netdev_stats_t* out_stats) {
    if (if_id >= g_num_interfaces || !out_stats) {
        return -1; /* Not found */
    }

    *out_stats = g_interfaces[if_id].stats;
    return 0;
}

net_if_t* net_get_iface(uint32_t if_id) {
    if (if_id >= g_num_interfaces) {
        return NULL;
    }
    return &g_interfaces[if_id];
}
