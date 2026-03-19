#ifndef NETMGR_DRIVER_POLICY_H
#define NETMGR_DRIVER_POLICY_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>
#include <bharat/network/netmgr_ipc.h>

#define NETMGR_MAX_DRIVER_PROFILES 8

typedef enum {
    PROFILE_TYPE_DEFAULT,
    PROFILE_TYPE_FAST_PATH,
    PROFILE_TYPE_PASSTHROUGH,
    PROFILE_TYPE_VIRTUAL
} driver_profile_type_t;

typedef struct {
    uint32_t profile_id;
    driver_profile_type_t type;
    bool allow_tso;         // TCP Segmentation Offload
    bool allow_csum;        // Checksum Offload
    bool allow_promisc;     // Promiscuous mode allowed
    uint32_t max_queues;
    bool valid;
} netmgr_driver_profile_t;

typedef struct {
    net_if_id_t if_id;
    uint32_t profile_id;
    bool valid;
} netmgr_driver_binding_t;

void netmgr_driver_policy_init(void);

netmgr_status_t netmgr_driver_policy_add_profile(uint32_t profile_id, driver_profile_type_t type, bool tso, bool csum, bool promisc, uint32_t max_queues);

netmgr_status_t netmgr_driver_policy_bind(net_if_id_t if_id, uint32_t profile_id);

netmgr_driver_profile_t* netmgr_driver_policy_get_profile(uint32_t profile_id);

netmgr_driver_profile_t* netmgr_driver_policy_get_binding(net_if_id_t if_id);

#endif // NETMGR_DRIVER_POLICY_H
