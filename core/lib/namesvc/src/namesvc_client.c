#include <bharat/namesvc/client.h>
#include <bharat/uapi/services/bootstrap.h>
#include <bharat/runtime/freestanding_string.h>

int namesvc_register(const char *name,
                     bharat_service_id_t service_id,
                     bharat_ipc_endpoint_t endpoint,
                     uint32_t version,
                     uint32_t flags) {
    if (!name) return NAMESVC_STATUS_ERR_INVAL;

    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = BHARAT_SERVICE_NAMESVC,
        .opcode = BHARAT_NAMESVC_OP_REGISTER,
        .payload_size = sizeof(namesvc_ipc_req_t),
        .capability_transfer = endpoint
    };

    namesvc_ipc_req_t req = {0};
    req.opcode = BHARAT_NAMESVC_OP_REGISTER;
    strncpy(req.u.reg.service_name, name, NAMESVC_MAX_NAME_LEN - 1);
    req.u.reg.service_id = service_id;
    req.u.reg.interface_version = version;
    req.u.reg.transport_flags = flags;

    bharat_ipc_msg_header_t res_hdr = {0};
    namesvc_ipc_res_t res = {0};

    int ret = bharat_ipc_call(BHARAT_BOOTSTRAP_NAMESVC_ENDPOINT, &req_hdr, &req, &res_hdr, &res, sizeof(res));
    if (ret != BHARAT_IPC_STATUS_OK) return ret;

    return res.status;
}

int namesvc_lookup(const char *name,
                   bharat_service_id_t *out_service_id,
                   bharat_ipc_endpoint_t *out_endpoint,
                   uint32_t *out_version) {
    if (!name) return NAMESVC_STATUS_ERR_INVAL;

    bharat_ipc_msg_header_t req_hdr = {
        .header_version = BHARAT_IPC_HEADER_VERSION_V1,
        .service_id = BHARAT_SERVICE_NAMESVC,
        .opcode = BHARAT_NAMESVC_OP_LOOKUP,
        .payload_size = sizeof(namesvc_ipc_req_t)
    };

    namesvc_ipc_req_t req = {0};
    req.opcode = BHARAT_NAMESVC_OP_LOOKUP;
    strncpy(req.u.lookup.service_name, name, NAMESVC_MAX_NAME_LEN - 1);

    bharat_ipc_msg_header_t res_hdr = {0};
    namesvc_ipc_res_t res = {0};

    int ret = bharat_ipc_call(BHARAT_BOOTSTRAP_NAMESVC_ENDPOINT, &req_hdr, &req, &res_hdr, &res, sizeof(res));
    if (ret != BHARAT_IPC_STATUS_OK) return ret;

    if (res.status == NAMESVC_STATUS_OK) {
        if (out_service_id) *out_service_id = res.u.lookup_res.service_id;
        if (out_endpoint) *out_endpoint = res_hdr.capability_transfer;
        if (out_version) *out_version = res.u.lookup_res.interface_version;
    }

    return res.status;
}
