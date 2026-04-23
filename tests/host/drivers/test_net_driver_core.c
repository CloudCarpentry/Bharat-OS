#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "drivers/net/net_driver.h"

static int g_tx_called;

static int test_probe(netdrv_device_t* dev, void* bus_device) {
    dev->bus_ctx = bus_device;
    return 0;
}

static int test_init(netdrv_device_t* dev) {
    dev->state = NETDRV_STATE_INITIALIZED;
    return 0;
}

static int test_start(netdrv_device_t* dev) {
    dev->state = NETDRV_STATE_STARTED;
    return netdrv_set_carrier(dev, true);
}

static int test_stop(netdrv_device_t* dev) {
    dev->state = NETDRV_STATE_STOPPED;
    return netdrv_set_carrier(dev, false);
}

static int test_tx(netdrv_device_t* dev, packet_buf_t* pkt, uint8_t queue_id) {
    (void)dev;
    (void)queue_id;
    g_tx_called++;
    packet_unref(pkt);
    return 0;
}

static int test_rx(netdrv_device_t* dev, packet_buf_t** out_pkt, uint8_t queue_id) {
    (void)dev;
    (void)queue_id;
    (void)out_pkt;
    return -1;
}

static int test_poll(netdrv_device_t* dev) {
    (void)dev;
    return 0;
}

static int test_irq(netdrv_device_t* dev, uint32_t irq_status) {
    (void)dev;
    (void)irq_status;
    return 0;
}

static int test_set_mtu(netdrv_device_t* dev, uint32_t mtu) {
    (void)dev;
    return mtu >= 576 && mtu <= 9000 ? 0 : -1;
}

static int test_set_promisc(netdrv_device_t* dev, bool enabled) {
    dev->promisc_enabled = enabled;
    return 0;
}

int main(void) {
    netdrv_device_t dev = {
        .name = "testnic0",
        .device_id = 3,
        .ops = &(netdrv_ops_t){
            .probe = test_probe,
            .init = test_init,
            .start = test_start,
            .stop = test_stop,
            .tx = test_tx,
            .rx = test_rx,
            .poll = test_poll,
            .irq = test_irq,
            .set_mtu = test_set_mtu,
            .set_promisc = test_set_promisc,
        },
        .caps = {
            .mtu = 1500,
            .min_mtu = 576,
            .max_mtu = 9000,
            .tx_queues = 1,
            .rx_queues = 1,
            .flags = NETDRV_CAP_POLL_FALLBACK,
        },
    };

    packet_buf_t* pkt;

    libpacket_init();
    assert(netdrv_register(&dev) == 0);
    assert(netdrv_get(3) == &dev);
    assert(test_start(&dev) == 0);

    pkt = packet_alloc();
    assert(pkt != NULL);
    pkt->data_len = 64;
    assert(netdrv_submit_tx(&dev, pkt, 0) == 0);
    assert(g_tx_called == 1);
    assert(dev.stats.tx_packets == 1);

    assert(netdrv_set_mtu(&dev, 9001) == -1);
    assert(netdrv_set_mtu(&dev, 2000) == 0);
    assert(dev.caps.mtu == 2000);

    assert(netdrv_handle_irq(&dev, 0x1) == 0);
    assert(dev.stats.irq_count == 1);

    assert(netdrv_unregister(&dev) == 0);
    printf("test_net_driver_core passed.\n");
    return 0;
}
