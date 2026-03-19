#ifndef NETMGR_IPC_DISPATCH_H
#define NETMGR_IPC_DISPATCH_H

#include <stdint.h>
#include <bharat/network/netmgr_ipc.h>

void netmgr_ipc_dispatch_init(void);

// Main dispatch loop or single handler entry point
void netmgr_ipc_handle_request(const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res);

#endif // NETMGR_IPC_DISPATCH_H
