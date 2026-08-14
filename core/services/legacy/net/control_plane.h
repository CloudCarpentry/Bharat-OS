#ifndef SERVICES_NET_CONTROL_PLANE_H
#define SERVICES_NET_CONTROL_PLANE_H

#include "net_types.h"
#include <bharat/uapi/service_status.h>

/*
 * Control Plane API
 *
 * Manages interfaces, link state, MTU, configuration, and stats querying.
 */

/* Initialize the control plane */
void net_control_plane_init(void);

/* Register a new interface into the interface table */
bharat_status_t net_register_interface(const char* name, const uint8_t* mac, uint32_t mtu, uint32_t* out_if_id);

/* Set the link state of an interface (Up/Down) */
bharat_status_t net_set_link_state(uint32_t if_id, net_link_state_t state);

/* Retrieve interface statistics */
bharat_status_t net_get_stats(uint32_t if_id, netdev_stats_t* out_stats);

/* Internal lookup used by data plane (ideally would be via capabilities/safe handoff) */
net_if_t* net_get_iface(uint32_t if_id);

#endif /* SERVICES_NET_CONTROL_PLANE_H */
