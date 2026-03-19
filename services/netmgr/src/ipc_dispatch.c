static void *memset(void *s, int c, unsigned long n) { unsigned char *p = s; while(n--) *p++ = (unsigned char)c; return s; }
static void *memcpy(void *dest, const void *src, unsigned long n) { unsigned char *d = dest; const unsigned char *s = src; while (n--) *d++ = *s++; return dest; }
#include "ipc_dispatch.h"
#include "interface_table.h"
#include "address_table.h"
#include "route_table.h"
#include "neighbor_cache.h"
#include "driver_policy.h"
#include "driver_health.h"
#include "capability_checks.h"

#include <stddef.h>

void netmgr_ipc_dispatch_init(void) {
    netmgr_iface_table_init();
    netmgr_addr_table_init();
    netmgr_route_table_init();
    netmgr_neighbor_cache_init();
    netmgr_driver_policy_init();
    netmgr_driver_health_init();
}

void netmgr_ipc_handle_request(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    memset(res, 0, sizeof(netmgr_ipc_res_t));
    res->status = NETMGR_STATUS_ERR_INVAL;

    if (!req) return;

    switch (req->opcode) {
        case NETMGR_OP_CREATE_IFACE: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_ADMIN, 0)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            net_if_id_t new_id = NET_IF_ID_INVALID;
            res->status = netmgr_iface_create(
                req->u.create_iface.name,
                req->u.create_iface.mac,
                req->u.create_iface.mtu,
                &new_id
            );
            if (res->status == NETMGR_STATUS_OK) {
                res->u.create_iface_res.if_id = new_id;
                netmgr_driver_health_register(new_id);
            }
            break;
        }

        case NETMGR_OP_DELETE_IFACE: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_ADMIN, req->u.delete_iface.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            netmgr_neighbor_flush(req->u.delete_iface.if_id);
            netmgr_driver_health_unregister(req->u.delete_iface.if_id);
            res->status = netmgr_iface_delete(req->u.delete_iface.if_id);
            break;
        }

        case NETMGR_OP_SET_ADMIN_STATE: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_WRITE, req->u.set_admin_state.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_iface_set_admin_state(req->u.set_admin_state.if_id, req->u.set_admin_state.admin_up);
            break;
        }

        case NETMGR_OP_QUERY_STATS: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_READ, req->u.query_stats.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            netmgr_iface_t *iface = netmgr_iface_get(req->u.query_stats.if_id);
            if (iface) {
                res->u.stats_res.rx_bytes = iface->stats.rx_bytes;
                res->u.stats_res.tx_bytes = iface->stats.tx_bytes;
                res->u.stats_res.rx_packets = iface->stats.rx_packets;
                res->u.stats_res.tx_packets = iface->stats.tx_packets;
                res->u.stats_res.rx_errors = iface->stats.rx_errors;
                res->u.stats_res.tx_errors = iface->stats.tx_errors;
                res->status = NETMGR_STATUS_OK;
            } else {
                res->status = NETMGR_STATUS_ERR_NOTFOUND;
            }
            break;
        }

        case NETMGR_OP_ADD_ADDR: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_WRITE, req->u.add_addr.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_addr_add(
                req->u.add_addr.if_id,
                req->u.add_addr.af,
                req->u.add_addr.addr,
                req->u.add_addr.prefix_len
            );
            break;
        }

        case NETMGR_OP_REMOVE_ADDR: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_WRITE, req->u.add_addr.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_addr_remove(
                req->u.add_addr.if_id,
                req->u.add_addr.af,
                req->u.add_addr.addr,
                req->u.add_addr.prefix_len
            );
            break;
        }

        case NETMGR_OP_ADD_ROUTE: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_WRITE, req->u.add_route.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_route_add(
                req->u.add_route.if_id,
                req->u.add_route.af,
                req->u.add_route.dest,
                req->u.add_route.mask,
                req->u.add_route.gateway,
                req->u.add_route.metric
            );
            break;
        }

        case NETMGR_OP_REMOVE_ROUTE: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_WRITE, 0)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_route_remove(
                req->u.add_route.af,
                req->u.add_route.dest,
                req->u.add_route.mask
            );
            break;
        }

        case NETMGR_OP_NEIGHBOR_FLUSH: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_ADMIN, req->u.restart_driver.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_neighbor_flush(req->u.restart_driver.if_id);
            break;
        }

        case NETMGR_OP_RESTART_DRIVER: {
            if (!netmgr_cap_check_rights(NETMGR_CAP_ADMIN, req->u.restart_driver.if_id)) {
                res->status = NETMGR_STATUS_ERR_PERM;
                break;
            }
            res->status = netmgr_driver_health_request_restart(req->u.restart_driver.if_id);
            break;
        }

        default:
            res->status = NETMGR_STATUS_ERR_INVAL;
            break;
    }
}
