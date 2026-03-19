#ifndef BHARAT_NETWORK_CONTROL_H
#define BHARAT_NETWORK_CONTROL_H

#include "types.h"
#include "errors.h"

/* Control-plane API contracts */

/* Interface Operations */
int bnet_ctrl_create_interface(bnet_mac_type_t type, bnet_interface_id_t *out_id);
int bnet_ctrl_delete_interface(bnet_interface_id_t id);

int bnet_ctrl_set_link_state(bnet_interface_id_t id, bnet_link_state_t state);
int bnet_ctrl_set_admin_state(bnet_interface_id_t id, bnet_link_state_t state);

/* Addressing */
int bnet_ctrl_assign_address(bnet_interface_id_t id, const bnet_ip_prefix_t *prefix);
int bnet_ctrl_remove_address(bnet_interface_id_t id, const bnet_ip_prefix_t *prefix);

/* Routing */
int bnet_ctrl_route_add(const bnet_route_key_t *route);
int bnet_ctrl_route_remove(const bnet_route_key_t *route);

/* Neighbors */
int bnet_ctrl_neighbor_add(bnet_interface_id_t id, const bnet_ip_addr_t *ip, const bnet_mac_addr_t *mac);
int bnet_ctrl_neighbor_remove(bnet_interface_id_t id, const bnet_ip_addr_t *ip);

/* Telemetry and Queries */
int bnet_ctrl_get_stats(bnet_interface_id_t id, bnet_stats_t *out_stats);
int bnet_ctrl_get_features(bnet_interface_id_t id, uint64_t *out_features);

#endif // BHARAT_NETWORK_CONTROL_H
