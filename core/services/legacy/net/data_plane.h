#ifndef SERVICES_NET_DATA_PLANE_H
#define SERVICES_NET_DATA_PLANE_H

#include "net_types.h"
#include <net/netdev.h>

/*
 * Data Plane API
 *
 * Manages frame RX/TX ownership, packet buffer abstractions, and interface stats updates.
 */

/* Initialize data plane */
void net_data_plane_init(void);

/* Submit a packet into the RX path */
int net_dp_rx_submit(uint32_t if_id, netbuf_t* buf);

/* Submit a packet into the TX path */
int net_dp_tx_submit(uint32_t if_id, netbuf_t* buf);

#endif /* SERVICES_NET_DATA_PLANE_H */
