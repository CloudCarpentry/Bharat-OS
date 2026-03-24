#include "ipc_ops.h"
#include "interface_table.h"
#include "address_table.h"
#include "route_table.h"
#include "neighbor_cache.h"
#include "driver_health.h"

uint32_t netmgr_extract_if_id_delete(const netmgr_ipc_req_t *req) { return req->u.delete_iface.if_id; }
uint32_t netmgr_extract_if_id_set_admin_state(const netmgr_ipc_req_t *req) { return req->u.set_admin_state.if_id; }
uint32_t netmgr_extract_if_id_query_stats(const netmgr_ipc_req_t *req) { return req->u.query_stats.if_id; }
uint32_t netmgr_extract_if_id_add_addr(const netmgr_ipc_req_t *req) { return req->u.add_addr.if_id; }
uint32_t netmgr_extract_if_id_remove_addr(const netmgr_ipc_req_t *req) { return req->u.remove_addr.if_id; }
uint32_t netmgr_extract_if_id_add_route(const netmgr_ipc_req_t *req) { return req->u.add_route.if_id; }
uint32_t netmgr_extract_if_id_neighbor_flush(const netmgr_ipc_req_t *req) { return req->u.neighbor_flush.if_id; }
uint32_t netmgr_extract_if_id_restart_driver(const netmgr_ipc_req_t *req) { return req->u.restart_driver.if_id; }
uint32_t netmgr_extract_if_id_none(const netmgr_ipc_req_t *req) { (void)req; return 0; }


void netmgr_op_create_iface(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
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
}

void netmgr_op_delete_iface(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    netmgr_neighbor_flush(req->u.delete_iface.if_id);
    netmgr_driver_health_unregister(req->u.delete_iface.if_id);
    res->status = netmgr_iface_delete(req->u.delete_iface.if_id);
}

void netmgr_op_set_admin_state(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_iface_set_admin_state(req->u.set_admin_state.if_id, req->u.set_admin_state.admin_up);
}

void netmgr_op_query_stats(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
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
}

void netmgr_op_add_addr(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_addr_add(
        req->u.add_addr.if_id,
        req->u.add_addr.af,
        req->u.add_addr.addr,
        req->u.add_addr.prefix_len
    );
}

void netmgr_op_remove_addr(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_addr_remove(
        req->u.remove_addr.if_id,
        req->u.remove_addr.af,
        req->u.remove_addr.addr,
        req->u.remove_addr.prefix_len
    );
}

void netmgr_op_add_route(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_route_add(
        req->u.add_route.if_id,
        req->u.add_route.af,
        req->u.add_route.dest,
        req->u.add_route.mask,
        req->u.add_route.gateway,
        req->u.add_route.metric
    );
}

void netmgr_op_remove_route(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_route_remove(
        req->u.remove_route.af,
        req->u.remove_route.dest,
        req->u.remove_route.mask
    );
}

void netmgr_op_neighbor_flush(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_neighbor_flush(req->u.neighbor_flush.if_id);
}

void netmgr_op_restart_driver(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    res->status = netmgr_driver_health_request_restart(req->u.restart_driver.if_id);
}
