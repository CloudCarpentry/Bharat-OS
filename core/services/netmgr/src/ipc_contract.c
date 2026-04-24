#include "ipc_contract.h"
#include "ipc_ops.h"
#include "ipc_auth.h"

static const netmgr_op_descriptor_t netmgr_op_descriptors[] = {
    {
        .opcode = NETMGR_OP_CREATE_IFACE,
        .name = "CREATE_IFACE",
        .min_request_size = sizeof(struct netmgr_req_create_iface),
        .max_request_size = sizeof(struct netmgr_req_create_iface),
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
        .extract_target_obj = netmgr_extract_if_id_none,
        .handler = netmgr_op_create_iface,
    },
    {
        .opcode = NETMGR_OP_DELETE_IFACE,
        .name = "DELETE_IFACE",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
        .extract_target_obj = netmgr_extract_if_id_delete,
        .handler = netmgr_op_delete_iface,
    },
    {
        .opcode = NETMGR_OP_SET_ADMIN_STATE,
        .name = "SET_ADMIN_STATE",
        .min_request_size = sizeof(struct netmgr_req_set_admin_state),
        .max_request_size = sizeof(struct netmgr_req_set_admin_state),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_CONFIG,
        .extract_target_obj = netmgr_extract_if_id_set_admin_state,
        .handler = netmgr_op_set_admin_state,
    },
    {
        .opcode = NETMGR_OP_QUERY_STATS,
        .name = "QUERY_STATS",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_READ_STATS,
        .extract_target_obj = netmgr_extract_if_id_query_stats,
        .handler = netmgr_op_query_stats,
    },
    {
        .opcode = NETMGR_OP_ADD_ADDR,
        .name = "ADD_ADDR",
        .min_request_size = sizeof(struct netmgr_req_add_addr),
        .max_request_size = sizeof(struct netmgr_req_add_addr),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_CONFIG,
        .extract_target_obj = netmgr_extract_if_id_add_addr,
        .handler = netmgr_op_add_addr,
    },
    {
        .opcode = NETMGR_OP_REMOVE_ADDR,
        .name = "REMOVE_ADDR",
        .min_request_size = sizeof(struct netmgr_req_add_addr), // implicitly uses add_addr layout
        .max_request_size = sizeof(struct netmgr_req_add_addr),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_IFACE_CONFIG,
        .extract_target_obj = netmgr_extract_if_id_remove_addr,
        .handler = netmgr_op_remove_addr,
    },
    {
        .opcode = NETMGR_OP_ADD_ROUTE,
        .name = "ADD_ROUTE",
        .min_request_size = sizeof(struct netmgr_req_add_route),
        .max_request_size = sizeof(struct netmgr_req_add_route),
        .object_type = BHARAT_CAP_OBJ_ROUTE_TABLE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ROUTE_MUTATE,
        .extract_target_obj = netmgr_extract_if_id_add_route,
        .handler = netmgr_op_add_route,
    },
    {
        .opcode = NETMGR_OP_REMOVE_ROUTE,
        .name = "REMOVE_ROUTE",
        .min_request_size = sizeof(struct netmgr_req_remove_route),
        .max_request_size = sizeof(struct netmgr_req_remove_route),
        .object_type = BHARAT_CAP_OBJ_ROUTE_TABLE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ROUTE_MUTATE,
        .extract_target_obj = netmgr_extract_if_id_none, // route removal currently doesn't check specific if_id
        .handler = netmgr_op_remove_route,
    },
    {
        .opcode = NETMGR_OP_NEIGHBOR_FLUSH,
        .name = "NEIGHBOR_FLUSH",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
        .extract_target_obj = netmgr_extract_if_id_neighbor_flush,
        .handler = netmgr_op_neighbor_flush,
    },
    {
        .opcode = NETMGR_OP_RESTART_DRIVER,
        .name = "RESTART_DRIVER",
        .min_request_size = sizeof(struct netmgr_req_iface_id),
        .max_request_size = sizeof(struct netmgr_req_iface_id),
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
        .extract_target_obj = netmgr_extract_if_id_restart_driver,
        .handler = netmgr_op_restart_driver,
    },
    {0, NULL, 0, 0, 0, 0, NULL, NULL} // Terminator
};

const netmgr_op_descriptor_t* netmgr_get_op_descriptor(uint32_t opcode) {
    for (int i = 0; netmgr_op_descriptors[i].name != NULL; ++i) {
        if (netmgr_op_descriptors[i].opcode == opcode) {
            return &netmgr_op_descriptors[i];
        }
    }
    return NULL;
}
