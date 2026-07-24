#include <ipc_dispatch.h>
#include <registry.h>
#include <ipc/contract_validate.h>
#include <ipc_auth.h>
#include <bharat/runtime/freestanding_string.h>

void namesvc_ipc_handle_request(const bharat_ipc_contract_header_t *hdr, const namesvc_ipc_req_t *req, namesvc_ipc_res_t *res) {
    memset(res, 0, sizeof(namesvc_ipc_res_t));

    if (!hdr || !req) {
        res->status = NAMESVC_STATUS_ERR_INVAL;
        return;
    }

    // Baseline validation of the contract header
    int validation_status = bharat_ipc_contract_validate(hdr, 1, BHARAT_NAMESVC_OP_REGISTER, BHARAT_NAMESVC_OP_LIST_INTERFACES, 0);
    if (validation_status != BHARAT_IPC_STATUS_OK) {
        if (validation_status == BHARAT_IPC_STATUS_ERR_VERSION) {
            res->status = BHARAT_IPC_STATUS_ERR_VERSION;
            return;
        }
        res->status = NAMESVC_STATUS_ERR_INVAL;
        return;
    }

    int auth_status = namesvc_authorize(req->opcode, hdr->capability_transfer);
    if (auth_status != BHARAT_IPC_STATUS_OK) {
        res->status = auth_status;
        return;
    }

    switch (req->opcode) {
        case BHARAT_NAMESVC_OP_REGISTER:
            res->status = namesvc_registry_add(
                req->u.reg.service_name,
                req->u.reg.service_id,
                req->u.reg.interface_version,
                req->u.reg.transport_flags,
                hdr->capability_transfer
            );
            break;

        case BHARAT_NAMESVC_OP_LOOKUP:
            res->status = namesvc_registry_lookup(
                req->u.lookup.service_name,
                req->u.lookup.requested_version,
                req->u.lookup.exact_version,
                &res->u.lookup_res.endpoint,
                &res->u.lookup_res.service_id,
                &res->u.lookup_res.interface_version,
                &res->u.lookup_res.transport_flags
            );
            break;

        case BHARAT_NAMESVC_OP_UNREGISTER:
            res->status = namesvc_registry_remove(
                req->u.unreg.service_name
            );
            break;

        case BHARAT_NAMESVC_OP_LIST_INTERFACES:
            res->status = NAMESVC_STATUS_ERR_UNSUPPORTED;
            break;

        default:
            res->status = NAMESVC_STATUS_ERR_INVAL;
            break;
    }
}
