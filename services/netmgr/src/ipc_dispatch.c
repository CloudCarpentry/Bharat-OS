#include "ipc_dispatch.h"
#include <bharat/runtime/freestanding_string.h>
#include "interface_table.h"
#include "address_table.h"
#include "route_table.h"
#include "neighbor_cache.h"
#include "driver_policy.h"
#include "driver_health.h"
#include "ipc_contract.h"
#include "ipc_auth.h"
#include <ipc/contract_validate.h>

#include <stddef.h>

void netmgr_ipc_dispatch_init(void) {
    netmgr_iface_table_init();
    netmgr_addr_table_init();
    netmgr_route_table_init();
    netmgr_neighbor_cache_init();
    netmgr_driver_policy_init();
    netmgr_driver_health_init();
}

void netmgr_ipc_handle_request(const bharat_ipc_msg_header_t *hdr, const netmgr_ipc_req_t *req, netmgr_ipc_res_t *res) {
    memset(res, 0, sizeof(netmgr_ipc_res_t));
    res->status = NETMGR_STATUS_ERR_INVAL;

    if (!req || !hdr) return;

    // Validate Contract First (Assuming Interface Version 1, Opcodes 1 to 42)
    int validation_status = bharat_ipc_contract_validate(hdr, 1, NETMGR_OP_CREATE_IFACE, NETMGR_OP_RESTART_DRIVER, 0);
    if (validation_status != BHARAT_IPC_STATUS_OK) {
        if (validation_status == BHARAT_IPC_STATUS_ERR_VERSION) {
            // Keep explicit version mismatch
            res->status = BHARAT_IPC_STATUS_ERR_VERSION;
            return;
        }
        res->status = NETMGR_STATUS_ERR_INVAL;
        return;
    }

    const netmgr_op_descriptor_t *desc = netmgr_get_op_descriptor(req->opcode);
    if (!desc) {
        // Unknown opcode
        res->status = NETMGR_STATUS_ERR_INVAL;
        return;
    }

    // Payload size validation
    if (hdr->payload_size < desc->min_request_size || hdr->payload_size > desc->max_request_size) {
        res->status = NETMGR_STATUS_ERR_INVAL;
        return;
    }

    // Set the capability token from the IPC header so the capability checker can evaluate it.
    netmgr_set_caller_cap(hdr->capability_transfer);

    uint32_t target_if_id = desc->extract_target_obj(req);
    int auth_status = netmgr_authorize(req->opcode, hdr->capability_transfer, target_if_id, desc->required_rights);

    if (auth_status != BHARAT_IPC_STATUS_OK) {
        res->status = NETMGR_STATUS_ERR_PERM;
        return;
    }

    // Run the handler
    desc->handler(req, res);
}
