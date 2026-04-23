#include <ipc_dispatch.h>
#include <registry.h>
#include <ipc/contract_validate.h>
#include <ipc_auth.h>

static void custom_memset_ipc(void *s, int c, unsigned long n) {
    unsigned char *p = s;
    while(n--) *p++ = (unsigned char)c;
}

void namesvc_ipc_handle_request(const bharat_ipc_contract_header_t *hdr, const namesvc_ipc_req_t *req, namesvc_ipc_res_t *res) {
    custom_memset_ipc(res, 0, sizeof(namesvc_ipc_res_t));

    if (!hdr || !req) {
        res->status = NAMESVC_STATUS_ERR_INVAL;
        return;
    }

    // Baseline validation of the contract header
    int validation_status = bharat_ipc_contract_validate(hdr, 1, NAMESVC_OP_REGISTER, NAMESVC_OP_LIST_INTERFACES, 0);
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
        case NAMESVC_OP_REGISTER:
            res->status = namesvc_registry_add(
                req->u.reg.service_name,
                req->u.reg.interface_name,
                req->u.reg.interface_version,
                req->u.reg.transport_flags,
                hdr->capability_transfer
            );
            break;

        case NAMESVC_OP_LOOKUP:
            res->status = namesvc_registry_lookup(
                req->u.lookup.service_name,
                req->u.lookup.interface_name,
                req->u.lookup.interface_version,
                req->u.lookup.exact_version,
                &res->u.lookup_res.endpoint,
                &res->u.lookup_res.interface_version,
                &res->u.lookup_res.transport_flags
            );
            break;

        case NAMESVC_OP_REMOVE:
            res->status = namesvc_registry_remove(
                req->u.remove.service_name,
                req->u.remove.interface_name,
                req->u.remove.interface_version
            );
            break;

        case NAMESVC_OP_LIST_INTERFACES:
            // Placeholder: currently not returning list data, just OK if service exists
            res->status = NAMESVC_STATUS_ERR_INVAL; // To be implemented later
            break;

        default:
            res->status = NAMESVC_STATUS_ERR_INVAL;
            break;
    }
}
