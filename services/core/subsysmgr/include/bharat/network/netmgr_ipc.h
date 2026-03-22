#ifndef BHARAT_NETWORK_NETMGR_IPC_H
#define BHARAT_NETWORK_NETMGR_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>

#define NETMGR_MAX_IF_NAME_LEN 16
#define NETMGR_MAX_MAC_LEN      6

typedef enum {
    NETMGR_OP_INVALID = 0,
    NETMGR_OP_CREATE_IFACE = 1,
    NETMGR_OP_DELETE_IFACE = 2,
    NETMGR_OP_SET_ADMIN_STATE = 3,
    NETMGR_OP_QUERY_IFACES = 4,
    NETMGR_OP_QUERY_STATS = 5,
    NETMGR_OP_ADD_ADDR = 10,
    NETMGR_OP_REMOVE_ADDR = 11,
    NETMGR_OP_ADD_ROUTE = 20,
    NETMGR_OP_REMOVE_ROUTE = 21,
    NETMGR_OP_NEIGHBOR_FLUSH = 30,
    NETMGR_OP_NEIGHBOR_QUERY = 31,
    NETMGR_OP_QUERY_DRIVER_POLICY = 40,
    NETMGR_OP_QUERY_DRIVER_HEALTH = 41,
    NETMGR_OP_RESTART_DRIVER = 42
} netmgr_op_t;

typedef enum {
    NETMGR_STATUS_OK = 0,
    NETMGR_STATUS_ERR_INVAL = -1,
    NETMGR_STATUS_ERR_PERM = -2,
    NETMGR_STATUS_ERR_NOTFOUND = -3,
    NETMGR_STATUS_ERR_NOSPACE = -4,
    NETMGR_STATUS_ERR_BUSY = -5
} netmgr_status_t;

struct netmgr_req_create_iface {
    char name[NETMGR_MAX_IF_NAME_LEN];
    uint8_t mac[NETMGR_MAX_MAC_LEN];
    uint32_t mtu;
};

struct netmgr_req_iface_id {
    net_if_id_t if_id;
};

struct netmgr_req_set_admin_state {
    net_if_id_t if_id;
    bool admin_up;
};

struct netmgr_req_add_addr {
    net_if_id_t if_id;
    net_af_t af;
    uint8_t addr[16];
    uint8_t prefix_len;
};

struct netmgr_req_add_route {
    net_if_id_t if_id;
    net_af_t af;
    uint8_t dest[16];
    uint8_t mask[16];
    uint8_t gateway[16];
    uint32_t metric;
};

typedef struct {
    uint32_t opcode;
    uint32_t reserved;
    union {
        struct netmgr_req_create_iface create_iface;
        struct netmgr_req_iface_id delete_iface;
        struct netmgr_req_set_admin_state set_admin_state;
        struct netmgr_req_iface_id query_stats;
        struct netmgr_req_add_addr add_addr;
        struct netmgr_req_add_route add_route;
        struct netmgr_req_iface_id restart_driver;
        uint8_t raw[112];
    } u;
} netmgr_ipc_req_t;

typedef struct {
    int32_t status;
    union {
        struct {
            net_if_id_t if_id;
        } create_iface_res;
        struct {
            uint64_t rx_bytes;
            uint64_t tx_bytes;
            uint32_t rx_packets;
            uint32_t tx_packets;
            uint32_t rx_errors;
            uint32_t tx_errors;
        } stats_res;
        uint8_t raw[124];
    } u;
} netmgr_ipc_res_t;

#endif
