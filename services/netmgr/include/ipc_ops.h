#ifndef NETMGR_IPC_OPS_H
#define NETMGR_IPC_OPS_H

#include <bharat/network/netmgr_ipc.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Handler definitions for each IPC operation
void netmgr_op_create_iface(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_delete_iface(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_set_admin_state(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_query_stats(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_add_addr(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_remove_addr(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_add_route(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_remove_route(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_neighbor_flush(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);
void netmgr_op_restart_driver(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);

// Helpers to extract target object ID
uint32_t netmgr_extract_if_id_delete(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_set_admin_state(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_query_stats(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_add_addr(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_remove_addr(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_add_route(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_neighbor_flush(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_restart_driver(const netmgr_ipc_req_t *req);
uint32_t netmgr_extract_if_id_none(const netmgr_ipc_req_t *req); // For CREATE, REMOVE_ROUTE, etc.

#ifdef __cplusplus
}
#endif

#endif // NETMGR_IPC_OPS_H
