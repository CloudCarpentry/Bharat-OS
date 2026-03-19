#include <stdint.h>
#include <stddef.h>

/* virtio_net.c
 *
 * Baseline virtio-net driver skeleton for BharatOS.
 * Target: First real NIC target.
 */

/* For Phase 2, this is a simplified/mockable driver backend.
   It doesn't implement full DMA/descriptor ring logic yet, but acts
   as the boundary for the netstack to interface with hardware eventually. */

static void (*netstack_rx_cb)(const uint8_t *data, size_t len) = NULL;

int virtio_net_init(void) {
    /* Initialize global driver state */
    return 0;
}

int virtio_net_probe(void *device) {
    /* Probe for PCI/MMIO virtio device */
    return 0;
}

int virtio_net_bind(void *device) {
    /* Bind driver to the hardware device */
    return 0;
}

int virtio_net_start(void *device, void (*rx_callback)(const uint8_t *, size_t)) {
    /* Configure rings, transition to DRIVER_OK, register RX callback */
    netstack_rx_cb = rx_callback;
    return 0;
}

int virtio_net_stop(void *device) {
    /* Halt queues, reset the device */
    netstack_rx_cb = NULL;
    return 0;
}

int virtio_net_tx(void *device, const void *buffer, size_t length) {
    /* Enqueue buffer into virtqueue for transmission.
       For the Phase 2 mock, we can just print or simulate sending. */
    return 0;
}

int virtio_net_poll(void *device) {
    /* Check ring status without interrupts and process pending packets */
    return 0;
}

/* Simulated RX for host testing/mocking in Phase 2 */
void virtio_net_mock_rx(const void *buffer, size_t length) {
    if (netstack_rx_cb) {
        netstack_rx_cb((const uint8_t *)buffer, length);
    }
}
