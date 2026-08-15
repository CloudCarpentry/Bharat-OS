#include "data_plane.h"
#include "control_plane.h"
#include <stddef.h>

/*
 * Data Plane Implementation
 */

void net_data_plane_init(void) {
    /* Ready the fast path/dispatch environment */
}

bharat_status_t net_dp_rx_submit(uint32_t if_id, netbuf_t* buf) {
    if (!buf) {
        return BHARAT_STATUS_ERR_INTERNAL;
    }

    net_if_t* iface = net_get_iface(if_id);
    if (!iface) {
        /* Drop frame if interface invalid */
        return BHARAT_STATUS_ERR_NOT_FOUND;
    }

    if (iface->link_state == NET_LINK_DOWN) {
        iface->stats.rx_dropped++;
        return BHARAT_STATUS_ERR_UNSUPPORTED; // Link down
    }

    /* Simulate RX process: Increment stats, pass to upper layers */
    iface->stats.rx_packets++;
    iface->stats.rx_bytes += buf->len;

    /* Here you would normally route to a net protocol stack handler */
    /* e.g., eth_input(buf); */

    return BHARAT_STATUS_OK;
}

bharat_status_t net_dp_tx_submit(uint32_t if_id, netbuf_t* buf) {
    if (!buf) {
        return BHARAT_STATUS_ERR_INTERNAL;
    }

    net_if_t* iface = net_get_iface(if_id);
    if (!iface) {
        /* Drop frame if interface invalid */
        return BHARAT_STATUS_ERR_NOT_FOUND;
    }

    if (iface->link_state == NET_LINK_DOWN) {
        iface->stats.tx_dropped++;
        return BHARAT_STATUS_ERR_UNSUPPORTED; // Link down
    }

    /* Simulate TX process: enqueue onto driver ring or just increment stats for loopback */
    iface->stats.tx_packets++;
    iface->stats.tx_bytes += buf->len;

    /* In a real stack, this would place the buffer onto the TX ring or URPC mailbox */

    return BHARAT_STATUS_OK;
}
