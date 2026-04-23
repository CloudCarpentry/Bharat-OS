#include <stdbool.h>
#include <stdint.h>

#include "drivers/can/can_controller.h"
#include "drivers/net/net_driver.h"

int net_can_contract_test_run(void) {
    netdrv_caps_t net_caps = {
        .link_up = false,
        .mtu = 1500,
        .min_mtu = 576,
        .max_mtu = 9000,
        .tx_queues = 1,
        .rx_queues = 1,
    };

    can_controller_caps_t can_caps = {
        .classical_can = true,
        .can_fd = true,
        .loopback = true,
        .listen_only = true,
        .min_bitrate = 10000,
        .max_bitrate = 1000000,
        .min_data_bitrate = 10000,
        .max_data_bitrate = 2000000,
    };

    if (net_caps.mtu < net_caps.min_mtu || net_caps.mtu > net_caps.max_mtu) {
        return -1;
    }
    if (!can_caps.classical_can || can_caps.max_bitrate < can_caps.min_bitrate) {
        return -1;
    }
    return 0;
}
