#ifndef NETMGR_DRIVER_HEALTH_H
#define NETMGR_DRIVER_HEALTH_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/net/net.h>
#include <bharat/network/netmgr_ipc.h>

typedef enum {
    DRIVER_HEALTH_UNKNOWN,
    DRIVER_HEALTH_OK,
    DRIVER_HEALTH_DEGRADED,
    DRIVER_HEALTH_FAILED,
    DRIVER_HEALTH_RESTARTING
} netmgr_driver_health_state_t;

typedef struct {
    net_if_id_t if_id;
    netmgr_driver_health_state_t state;
    uint32_t restart_count;
    uint64_t last_heartbeat;
    bool valid;
} netmgr_driver_health_t;

void netmgr_driver_health_init(void);

netmgr_status_t netmgr_driver_health_register(net_if_id_t if_id);

netmgr_status_t netmgr_driver_health_unregister(net_if_id_t if_id);

netmgr_status_t netmgr_driver_health_report(net_if_id_t if_id, netmgr_driver_health_state_t state);

netmgr_status_t netmgr_driver_health_request_restart(net_if_id_t if_id);

netmgr_driver_health_t* netmgr_driver_health_get(net_if_id_t if_id);

#endif // NETMGR_DRIVER_HEALTH_H
