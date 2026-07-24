#include "service_manifest.h"
#include <bharat/namesvc/namesvc_ipc.h>

// Static definition of operations for namesvc
static const bharat_ipc_op_manifest_t namesvc_ops_manifest[] = {
    {
        .opcode = NAMESVC_OP_REGISTER,
        .name = "REGISTER",
        .min_request_size = sizeof(struct namesvc_req_register),
        .max_request_size = sizeof(struct namesvc_req_register),
        .response_size = sizeof(namesvc_ipc_res_t),
        .required_rights = 0, // No specific right needed for now
    },
    {
        .opcode = NAMESVC_OP_LOOKUP,
        .name = "LOOKUP",
        .min_request_size = sizeof(struct namesvc_req_lookup),
        .max_request_size = sizeof(struct namesvc_req_lookup),
        .response_size = sizeof(namesvc_ipc_res_t),
        .required_rights = 0,
    },
    {
        .opcode = NAMESVC_OP_REMOVE,
        .name = "REMOVE",
        .min_request_size = sizeof(struct namesvc_req_remove),
        .max_request_size = sizeof(struct namesvc_req_remove),
        .response_size = sizeof(namesvc_ipc_res_t),
        .required_rights = 0,
    },
    {
        .opcode = NAMESVC_OP_LIST_INTERFACES,
        .name = "LIST_INTERFACES",
        .min_request_size = sizeof(struct namesvc_req_list_interfaces),
        .max_request_size = sizeof(struct namesvc_req_list_interfaces),
        .response_size = sizeof(namesvc_ipc_res_t),
        .required_rights = 0,
    }
};

const bharat_ipc_service_manifest_t namesvc_service_manifest = {
    .service_name = "namesvc",
    .interface_name = "Discovery",
    .interface_version = 1,
    .transport_kind = 1, // endpoint IPC
    .operation_count = sizeof(namesvc_ops_manifest) / sizeof(namesvc_ops_manifest[0]),
    .operations = namesvc_ops_manifest,
};
