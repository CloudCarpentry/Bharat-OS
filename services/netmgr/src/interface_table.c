static void *memset(void *s, int c, unsigned long n) { unsigned char *p = s; while(n--) *p++ = (unsigned char)c; return s; }
static void *memcpy(void *dest, const void *src, unsigned long n) { unsigned char *d = dest; const unsigned char *s = src; while (n--) *d++ = *s++; return dest; }
#include "interface_table.h"

#include <stddef.h>

static netmgr_iface_t g_interfaces[NETMGR_MAX_INTERFACES];
static uint32_t g_num_interfaces = 0;
static net_if_id_t g_next_if_id = 1;

void netmgr_iface_table_init(void) {
    g_num_interfaces = 0;
    g_next_if_id = 1;
    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        g_interfaces[i].if_id = NET_IF_ID_INVALID;
        g_interfaces[i].admin_up = false;
        g_interfaces[i].link_state = NET_LINK_DOWN;
        memset(&g_interfaces[i].stats, 0, sizeof(netmgr_iface_stats_t));
    }
}

netmgr_status_t netmgr_iface_create(const char *name, const uint8_t *mac, uint32_t mtu, net_if_id_t *out_if_id) {
    if (g_num_interfaces >= NETMGR_MAX_INTERFACES) {
        return NETMGR_STATUS_ERR_NOSPACE;
    }

    int free_slot = -1;
    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_interfaces[i].if_id == NET_IF_ID_INVALID) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) {
        return NETMGR_STATUS_ERR_NOSPACE;
    }

    netmgr_iface_t *iface = &g_interfaces[free_slot];
    iface->if_id = g_next_if_id++;

    int i = 0;
    while (name && name[i] != '\0' && i < NETMGR_MAX_IF_NAME_LEN - 1) {
        iface->name[i] = name[i];
        i++;
    }
    iface->name[i] = '\0';

    if (mac) {
        memcpy(iface->mac, mac, NETMGR_MAX_MAC_LEN);
    } else {
        memset(iface->mac, 0, NETMGR_MAX_MAC_LEN);
    }

    iface->mtu = mtu;
    iface->admin_up = false;
    iface->link_state = NET_LINK_DOWN;
    memset(&iface->stats, 0, sizeof(netmgr_iface_stats_t));

    g_num_interfaces++;

    if (out_if_id) {
        *out_if_id = iface->if_id;
    }

    return NETMGR_STATUS_OK;
}

netmgr_status_t netmgr_iface_delete(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) return NETMGR_STATUS_ERR_INVAL;

    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_interfaces[i].if_id == if_id) {
            g_interfaces[i].if_id = NET_IF_ID_INVALID;
            g_num_interfaces--;
            return NETMGR_STATUS_OK;
        }
    }

    return NETMGR_STATUS_ERR_NOTFOUND;
}

netmgr_status_t netmgr_iface_set_admin_state(net_if_id_t if_id, bool admin_up) {
    netmgr_iface_t *iface = netmgr_iface_get(if_id);
    if (!iface) return NETMGR_STATUS_ERR_NOTFOUND;
    iface->admin_up = admin_up;
    return NETMGR_STATUS_OK;
}

netmgr_iface_t* netmgr_iface_get(net_if_id_t if_id) {
    if (if_id == NET_IF_ID_INVALID) return NULL;

    for (int i = 0; i < NETMGR_MAX_INTERFACES; i++) {
        if (g_interfaces[i].if_id == if_id) {
            return &g_interfaces[i];
        }
    }

    return NULL;
}
