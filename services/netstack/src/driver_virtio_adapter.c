#include "driver_virtio_adapter.h"
#include "ethernet.h"
#include <stddef.h>
#include <bharat/runtime/freestanding_string.h>

#include <bharat/packet/packet.h>

// Forward declare the driver entry points for Phase 2 integration
extern int virtio_net_init(void);
extern int virtio_net_probe(void *device);
extern int virtio_net_bind(void *device);
extern int virtio_net_start(void *device, void (*rx_callback)(packet_buf_t *));
extern int virtio_net_tx(void *device, packet_buf_t *pkt);

// For simplicity, we assume a single device instance bound in the mock
static void *mock_virtio_device = NULL;

void virtio_adapter_rx(packet_buf_t *pkt) {
    size_t len = pkt->data_len;
    if (len > NETBUF_MAX_SIZE - NETBUF_DEFAULT_HEADROOM * 2) {
        packet_unref(pkt);
        return; // Dropped, too large
    }

    netbuf_t nb;
    netbuf_init(&nb);

    uint8_t *buf = netbuf_put(&nb, len);
    if (!buf) {
        packet_unref(pkt);
        return;
    }
    memcpy(buf, pkt->data, len);

    // Propagate hardware checksum metadata from driver to stack.
    // NOTE: PACKET_FLAG_RX_CSUM_VALID implies hardware validated the relevant
    // checksums for this packet type (e.g., IPv4/TCP/UDP). The stack layer must
    // only trust this for unmodified originally-received headers.
    // In a real driver, the NIC provides fine-grained flags.
    if (pkt->flags & PACKET_FLAG_RX_CSUM_VALID) {
        nb.flags |= NETBUF_F_RX_IP_CSUM_OK;
        nb.flags |= NETBUF_F_RX_L4_CSUM_OK;
    }

    packet_unref(pkt);

    // Enter the network stack
    ethernet_rx(&nb);
}

int virtio_adapter_init(void) {
    // Make sure libpacket is initialized before driver
    libpacket_init();

    if (virtio_net_init() != 0) return -1;
    if (virtio_net_probe(mock_virtio_device) != 0) return -1;
    if (virtio_net_bind(mock_virtio_device) != 0) return -1;
    if (virtio_net_start(mock_virtio_device, virtio_adapter_rx) != 0) return -1;
    return 0;
}

int virtio_adapter_tx(netbuf_t *nb) {
    packet_buf_t *pkt = packet_alloc();
    if (!pkt) return -1;

    size_t len = netbuf_len(nb);
    if (len > pkt->tail_len) {
        packet_free(pkt);
        return -1;
    }

    memcpy(pkt->data, netbuf_data(nb), len);
    pkt->data_len = len;

    // Translate stack-level TX offload intent to driver packet flags
    if ((nb->flags & NETBUF_F_TX_IP_CSUM_PARTIAL) || (nb->flags & NETBUF_F_TX_L4_CSUM_PARTIAL)) {
        pkt->flags |= PACKET_FLAG_TX_CSUM_REQ;
    }

    return virtio_net_tx(mock_virtio_device, pkt);
}
