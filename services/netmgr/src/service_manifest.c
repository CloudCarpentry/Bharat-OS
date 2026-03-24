#include "service_manifest.h"
#include <bharat/network/netmgr_ipc.h>
#include "ipc_auth.h"

// Static definition of operations matching the descriptor table
static const bharat_ipc_op_manifest_t netmgr_ops_manifest[] = {
    {
        .opcode = NETMGR_OP_CREATE_IFACE,
        .name = "CREATE_IFACE",
        .min_request_size = sizeof(struct netmgr_req_create_iface),
        .max_request_size = sizeof(struct netmgr_req_create_iface),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_ADMIN,
    },
    {
        .opcode = NETMGR_OP_DELETE_IFACE,
        .name = "DELETE_IFACE",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_ADMIN,
    },
    {
        .opcode = NETMGR_OP_SET_ADMIN_STATE,
        .name = "SET_ADMIN_STATE",
        .min_request_size = sizeof(struct netmgr_req_set_admin_state),
        .max_request_size = sizeof(struct netmgr_req_set_admin_state),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_WRITE,
    },
    {
        .opcode = NETMGR_OP_QUERY_STATS,
        .name = "QUERY_STATS",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_READ,
    },
    {
        .opcode = NETMGR_OP_ADD_ADDR,
        .name = "ADD_ADDR",
        .min_request_size = sizeof(struct netmgr_req_add_addr),
        .max_request_size = sizeof(struct netmgr_req_add_addr),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_WRITE,
    },
    {
        .opcode = NETMGR_OP_REMOVE_ADDR,
        .name = "REMOVE_ADDR",
        .min_request_size = sizeof(struct netmgr_req_add_addr), // Using add_addr format
        .max_request_size = sizeof(struct netmgr_req_add_addr),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_WRITE,
    },
    {
        .opcode = NETMGR_OP_ADD_ROUTE,
        .name = "ADD_ROUTE",
        .min_request_size = sizeof(struct netmgr_req_add_route),
        .max_request_size = sizeof(struct netmgr_req_add_route),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_WRITE,
    },
    {
        .opcode = NETMGR_OP_REMOVE_ROUTE,
        .name = "REMOVE_ROUTE",
        .min_request_size = sizeof(struct netmgr_req_remove_route),
        .max_request_size = sizeof(struct netmgr_req_remove_route),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_WRITE,
    },
    {
        .opcode = NETMGR_OP_NEIGHBOR_FLUSH,
        .name = "NEIGHBOR_FLUSH",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_ADMIN,
    },
    {
        .opcode = NETMGR_OP_RESTART_DRIVER,
        .name = "RESTART_DRIVER",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .response_size = sizeof(netmgr_ipc_res_t),
        .required_rights = NETMGR_RIGHT_ADMIN,
    }
};

const bharat_ipc_service_manifest_t netmgr_service_manifest = {
    .service_name = "netmgr",
    .interface_name = "NetworkControl",
    .interface_version = 1,
    .transport_kind = 1, // endpoint IPC
    .operation_count = sizeof(netmgr_ops_manifest) / sizeof(netmgr_ops_manifest[0]),
    .operations = netmgr_ops_manifest,
};
