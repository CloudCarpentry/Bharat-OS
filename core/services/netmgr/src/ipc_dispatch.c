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
    res->status = NETMGR_STATUS_ERR_PERM; // Default to PERM for deny-by-default

    if (!req || !hdr) return;

    // 1. Contract Lookup First
    const netmgr_op_descriptor_t *desc = netmgr_get_op_descriptor(req->opcode);
    if (!desc) {
        // Unknown opcode or missing contract - Deny by default
        // We use PERM here to avoid leaking information about opcode existence
        res->status = NETMGR_STATUS_ERR_PERM;
        return;
    }

    // 2. Validate IPC Contract/Version (Version 1)
    int validation_status = bharat_ipc_contract_validate(hdr, 1, NETMGR_OP_CREATE_IFACE, NETMGR_OP_RESTART_DRIVER, 0);
    if (validation_status != BHARAT_IPC_STATUS_OK) {
        if (validation_status == BHARAT_IPC_STATUS_ERR_VERSION) {
            res->status = BHARAT_IPC_STATUS_ERR_VERSION;
        } else {
            res->status = NETMGR_STATUS_ERR_INVAL;
        }
        return;
    }

    // 3. Payload size validation
    if (hdr->payload_size < desc->core.min_request_size || hdr->payload_size > desc->core.max_request_size) {
        res->status = NETMGR_STATUS_ERR_INVAL;
        return;
    }

    // 4. Authorization
    uint32_t target_id = desc->extract_target_obj(req);

    /* Convert to core capability kind for the authorization context */
    bh_cap_scope_t required_scope;
    if (desc->core.required_object_type == BHARAT_CAP_OBJ_SERVICE) {
        required_scope.kind = BH_CAP_SCOPE_SERVICE_KIND;
        required_scope.scope_id = 0;
    } else {
        required_scope.kind = BH_CAP_SCOPE_OBJECT_KIND;
        required_scope.scope_id = target_id;
    }

    /* Wrap the Bharat capability handle into an authorization context */
    /* In a real service, this might involve a cache lookup or a syscall to validate the handle */
    netmgr_auth_context_t auth_ctx = {0};
    bharat_cap_validation_result_t vr = {0};

    /* Standard Bharat capability validation syscall/lib wrapper */
    bharat_cap_status_t cap_st = bharat_cap_validate(
        hdr->capability_transfer,
        desc->core.required_object_type,
        target_id,
        desc->core.required_rights,
        (const bharat_cap_scope_t *)&required_scope,
        &vr
    );

    auth_ctx.handle = hdr->capability_transfer;
    auth_ctx.object_type = desc->core.required_object_type;
    auth_ctx.rights = vr.descriptor.rights;
    auth_ctx.scope.kind = (bh_cap_scope_kind_t)vr.descriptor.scope.kind;
    auth_ctx.scope.scope_id = vr.descriptor.scope.scope_id;
    auth_ctx.valid = (cap_st == BHARAT_CAP_OK && vr.allowed);
    auth_ctx.stale = (cap_st == BHARAT_CAP_STALE);
    auth_ctx.revoked = (cap_st == BHARAT_CAP_REVOKED);

    netmgr_auth_audit_t audit = {0};
    netmgr_auth_status_t auth_status = netmgr_ipc_authorize_contract(
        &desc->core,
        &auth_ctx,
        &required_scope,
        &audit
    );

    if (auth_status != NETMGR_AUTH_OK) {
        /* Authorization failed - call the unified audit hook */
        netmgr_audit_core_failure(&audit);
        res->status = NETMGR_STATUS_ERR_PERM;
        return;
    }

    // 5. Execution (Handled only after successful authorization)
    if (!desc->core.implemented || !desc->handler) {
        res->status = NETMGR_STATUS_ERR_UNSUPPORTED;
        return;
    }

    desc->handler(req, res);
}
