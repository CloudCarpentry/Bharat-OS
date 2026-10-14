#include "ipc_dispatch.h"
#include "registry.h"
#include <bharat/uapi/ipc/status.h>
#include <string.h>

void namesvc_ipc_handle_request(const bharat_ipc_contract_header_t *hdr,
                                const namesvc_ipc_req_t *req,
                                namesvc_ipc_res_t *res) {
    memset(res, 0, sizeof(*res));

    if (hdr->opcode != req->opcode) {
        res->status = BHARAT_IPC_STATUS_ERR_INVALID;
        return;
    }

    switch (hdr->opcode) {
        case BHARAT_NAMESVC_OP_LOOKUP: {
            if (hdr->payload_size != sizeof(struct namesvc_req_lookup)) {
                res->status = BHARAT_IPC_STATUS_ERR_INVALID;
                return;
            }
            int32_t r = namesvc_registry_lookup(req->u.lookup.service_name,
                                                req->u.lookup.requested_version,
                                                req->u.lookup.exact_version,
                                                &res->u.lookup_res.endpoint,
                                                &res->u.lookup_res.service_id,
                                                &res->u.lookup_res.interface_version,
                                                &res->u.lookup_res.transport_flags);
            if (r == NAMESVC_STATUS_OK) {
                res->status = BHARAT_IPC_STATUS_OK;
            } else {
                res->status = NAMESVC_STATUS_ERR_NOTFOUND;
            }
            break;
        }
        case BHARAT_NAMESVC_OP_REGISTER: {
            if (hdr->payload_size != sizeof(struct namesvc_req_register)) {
                res->status = BHARAT_IPC_STATUS_ERR_INVALID;
                return;
            }
            int32_t r = namesvc_registry_add(req->u.reg.service_name,
                                             req->u.reg.service_id,
                                             req->u.reg.interface_version,
                                             req->u.reg.transport_flags,
                                             hdr->capability_transfer);
            if (r == NAMESVC_STATUS_OK) {
                res->status = BHARAT_IPC_STATUS_OK;
            } else if (r == NAMESVC_STATUS_ERR_INVAL || r == NAMESVC_STATUS_ERR_PERM) {
                res->status = BHARAT_IPC_STATUS_ERR_PERM;
            } else {
                res->status = r; // Pass through EXISTS or NOSPACE
            }
            break;
        }
        case BHARAT_NAMESVC_OP_UNREGISTER: {
            if (hdr->payload_size != sizeof(struct namesvc_req_unregister)) {
                res->status = BHARAT_IPC_STATUS_ERR_INVALID;
                return;
            }
            int32_t r = namesvc_registry_remove(req->u.unreg.service_name, hdr->capability_transfer);
            if (r == NAMESVC_STATUS_OK) {
                res->status = BHARAT_IPC_STATUS_OK;
            } else if (r == NAMESVC_STATUS_ERR_PERM) {
                res->status = BHARAT_IPC_STATUS_ERR_PERM;
            } else {
                res->status = NAMESVC_STATUS_ERR_NOTFOUND;
            }
            break;
        }
        case BHARAT_NAMESVC_OP_LIST_INTERFACES: {
            res->status = BHARAT_IPC_STATUS_ERR_UNSUPPORTED; // Not fully implemented
            break;
        }
        default:
            res->status = BHARAT_IPC_STATUS_ERR_UNSUPPORTED;
            break;
    }
}
