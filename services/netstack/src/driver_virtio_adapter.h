#ifndef NETSTACK_DRIVER_VIRTIO_ADAPTER_H
#define NETSTACK_DRIVER_VIRTIO_ADAPTER_H

#include "netbuf.h"

/* Initialize the virtio adapter and bind it to the driver backend */
int virtio_adapter_init(void);

/* Transmit a buffer through the virtio driver */
int virtio_adapter_tx(netbuf_t *nb);

/* The RX callback from the virtio driver */
void virtio_adapter_rx(const uint8_t *data, size_t len);

#endif // NETSTACK_DRIVER_VIRTIO_ADAPTER_H
