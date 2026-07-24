#include "ipc_contract.h"
#include "ipc_ops.h"
#include "ipc_auth.h"

static const netmgr_op_descriptor_t netmgr_op_descriptors[] = {
    {
        .core = {
            .opcode = NETMGR_OP_CREATE_IFACE,
            .name = "CREATE_IFACE",
            .min_request_size = sizeof(struct netmgr_req_create_iface),
            .max_request_size = sizeof(struct netmgr_req_create_iface),
            .required_object_type = BHARAT_CAP_OBJ_SERVICE,
            .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_none,
        .handler = netmgr_op_create_iface,
    },
    {
        .core = {
            .opcode = NETMGR_OP_DELETE_IFACE,
            .name = "DELETE_IFACE",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_delete,
        .handler = netmgr_op_delete_iface,
    },
    {
        .core = {
            .opcode = NETMGR_OP_SET_ADMIN_STATE,
            .name = "SET_ADMIN_STATE",
            .min_request_size = sizeof(struct netmgr_req_set_admin_state),
            .max_request_size = sizeof(struct netmgr_req_set_admin_state),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_CONFIG,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_set_admin_state,
        .handler = netmgr_op_set_admin_state,
    },
    {
        .core = {
            .opcode = NETMGR_OP_QUERY_IFACES,
            .name = "QUERY_IFACES",
            .min_request_size = 0,
            .max_request_size = 0,
            .required_object_type = BHARAT_CAP_OBJ_SERVICE,
            .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_QUERY,
            .implemented = false,
        },
        .extract_target_obj = netmgr_extract_if_id_none,
        .handler = NULL,
    },
    {
        .core = {
            .opcode = NETMGR_OP_QUERY_STATS,
            .name = "QUERY_STATS",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_READ_STATS,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_query_stats,
        .handler = netmgr_op_query_stats,
    },
    {
        .core = {
            .opcode = NETMGR_OP_ADD_ADDR,
            .name = "ADD_ADDR",
            .min_request_size = sizeof(struct netmgr_req_add_addr),
            .max_request_size = sizeof(struct netmgr_req_add_addr),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_CONFIG,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_add_addr,
        .handler = netmgr_op_add_addr,
    },
    {
        .core = {
            .opcode = NETMGR_OP_REMOVE_ADDR,
            .name = "REMOVE_ADDR",
            .min_request_size = sizeof(struct netmgr_req_add_addr),
            .max_request_size = sizeof(struct netmgr_req_add_addr),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_CONFIG,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_remove_addr,
        .handler = netmgr_op_remove_addr,
    },
    {
        .core = {
            .opcode = NETMGR_OP_ADD_ROUTE,
            .name = "ADD_ROUTE",
            .min_request_size = sizeof(struct netmgr_req_add_route),
            .max_request_size = sizeof(struct netmgr_req_add_route),
            .required_object_type = BHARAT_CAP_OBJ_ROUTE_TABLE,
            .required_rights = BHARAT_CAP_RIGHT_NET_ROUTE_MUTATE,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_add_route,
        .handler = netmgr_op_add_route,
    },
    {
        .core = {
            .opcode = NETMGR_OP_REMOVE_ROUTE,
            .name = "REMOVE_ROUTE",
            .min_request_size = sizeof(struct netmgr_req_remove_route),
            .max_request_size = sizeof(struct netmgr_req_remove_route),
            .required_object_type = BHARAT_CAP_OBJ_ROUTE_TABLE,
            .required_rights = BHARAT_CAP_RIGHT_NET_ROUTE_MUTATE,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_none,
        .handler = netmgr_op_remove_route,
    },
    {
        .core = {
            .opcode = NETMGR_OP_NEIGHBOR_FLUSH,
            .name = "NEIGHBOR_FLUSH",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_neighbor_flush,
        .handler = netmgr_op_neighbor_flush,
    },
    {
        .core = {
            .opcode = NETMGR_OP_NEIGHBOR_QUERY,
            .name = "NEIGHBOR_QUERY",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_NEIGHBOR_QUERY,
            .implemented = false,
        },
        .extract_target_obj = netmgr_extract_if_id_neighbor_query,
        .handler = NULL,
    },
    {
        .core = {
            .opcode = NETMGR_OP_QUERY_DRIVER_POLICY,
            .name = "QUERY_DRIVER_POLICY",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_DRIVER_QUERY,
            .implemented = false,
        },
        .extract_target_obj = netmgr_extract_if_id_query_driver_policy,
        .handler = NULL,
    },
    {
        .core = {
            .opcode = NETMGR_OP_QUERY_DRIVER_HEALTH,
            .name = "QUERY_DRIVER_HEALTH",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_QUERY,
            .implemented = false,
        },
        .extract_target_obj = netmgr_extract_if_id_query_driver_health,
        .handler = NULL,
    },
    {
        .core = {
            .opcode = NETMGR_OP_RESTART_DRIVER,
            .name = "RESTART_DRIVER",
            .min_request_size = sizeof(struct netmgr_req_iface_id),
            .max_request_size = sizeof(struct netmgr_req_iface_id),
            .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
            .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
            .implemented = true,
        },
        .extract_target_obj = netmgr_extract_if_id_restart_driver,
        .handler = netmgr_op_restart_driver,
    },
    { .core = { .name = NULL } } // Terminator
};

const netmgr_op_descriptor_t* netmgr_get_op_descriptor(uint32_t opcode) {
    for (int i = 0; netmgr_op_descriptors[i].core.name != NULL; ++i) {
        if (netmgr_op_descriptors[i].core.opcode == opcode) {
            return &netmgr_op_descriptors[i];
        }
    }
    return NULL;
}
