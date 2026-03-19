#include "driver_virtio_adapter.h"
#include "ethernet.h"
#include <stddef.h>

// Forward declare the driver entry points for Phase 2 integration
extern int virtio_net_init(void);
extern int virtio_net_probe(void *device);
extern int virtio_net_bind(void *device);
extern int virtio_net_start(void *device, void (*rx_callback)(const uint8_t *, size_t));
extern int virtio_net_tx(void *device, const void *buffer, size_t length);

// For simplicity, we assume a single device instance bound in the mock
static void *mock_virtio_device = NULL;

void virtio_adapter_rx(const uint8_t *data, size_t len) {
    if (len > NETBUF_MAX_SIZE - NETBUF_DEFAULT_HEADROOM * 2) {
        return; // Dropped, too large
    }

    netbuf_t nb;
    netbuf_init(&nb);

    uint8_t *buf = netbuf_put(&nb, len);
    if (!buf) return;
    memcpy(buf, data, len);

    // Enter the network stack
    ethernet_rx(&nb);
}

int virtio_adapter_init(void) {
    if (virtio_net_init() != 0) return -1;
    if (virtio_net_probe(mock_virtio_device) != 0) return -1;
    if (virtio_net_bind(mock_virtio_device) != 0) return -1;
    if (virtio_net_start(mock_virtio_device, virtio_adapter_rx) != 0) return -1;
    return 0;
}

int virtio_adapter_tx(netbuf_t *nb) {
    return virtio_net_tx(mock_virtio_device, netbuf_data(nb), netbuf_len(nb));
}
