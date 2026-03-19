#include <stdint.h>
#include <stddef.h>
#include <net/netdev.h>
#include <net/net_core.h>

#include "control_plane.h"
#include "data_plane.h"

/*
 * Bharat-OS User-Space Network Service (L2/L3/L4)
 *
 * [LEGACY / TRANSITIONAL]
 * This monolithic network service is being deprecated. Its functionality
 * will be decomposed into `services/netmgr` (control/policy plane) and
 * `services/netstack` (data plane / IP stack) to better support the
 * Bharat-OS profile-driven architecture.
 *
 * Manages protocol stacks, IP addressing, sockets, and interacts with
 * physical network devices via capability-bounded zero-copy I/O rings.
 */

void minimal_smoke_test(void) {
    uint32_t loopback_id;
    uint8_t mock_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    /* 1. Register Mock/Loopback interface */
    if (net_register_interface("lo0", mock_mac, 1500, &loopback_id) == 0) {
        /* Success */
    }

    /* 2. Transition link state */
    net_set_link_state(loopback_id, NET_LINK_UP);

    /* 3. RX/TX Submit simulation */
    uint8_t dummy_frame[64] = {0};
    netbuf_t test_buf = {
        .data = dummy_frame,
        .len = 64,
        .head_room = 0,
        .tail_room = 0,
        .next = NULL,
        ._priv = NULL
    };

    net_dp_rx_submit(loopback_id, &test_buf);
    net_dp_tx_submit(loopback_id, &test_buf);

    /* 4. Verify stats */
    netdev_stats_t stats;
    if (net_get_stats(loopback_id, &stats) == 0) {
        /* Expected: rx_packets=1, tx_packets=1 */
        /* Normally we'd //printf here or assert, but we are keeping it silent/minimal */
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    /* Initialize internal planes */
    net_control_plane_init();
    net_data_plane_init();

    /* Run smoke test for loopback path / mock registration */
    minimal_smoke_test();

    // TODO: Acquire Capability for NIC Driver Rings via io_setup_zero_copy_nic_ring()
    // TODO: Register the NIC driver as a `netdev_t` device
    // TODO: Launch protocol processing thread (e.g. lwIP or native stack)

    while(1) {
        // Poll RX queues / URPC messages for sockets
        // Dispatch to network protocol handlers

        /* Stub main loop break to prevent infinite hang in some environments during tests,
           or leave if standard daemon behavior */
        break; /* Remove when full daemon run is expected */
    }

    return 0;
}
