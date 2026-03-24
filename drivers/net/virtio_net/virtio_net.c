#include <stdint.h>
#include <stddef.h>
#include <bharat/packet/packet.h>

/* virtio_net.c
 *
 * Baseline virtio-net driver skeleton for BharatOS.
 * Target: First real NIC target.
 */

/* For Phase 2, this is a simplified/mockable driver backend.
   It doesn't implement full DMA/descriptor ring logic yet, but acts
   as the boundary for the netstack to interface with hardware eventually. */

// VirtIO Network Features (Subset)
#define VIRTIO_NET_F_CSUM       (1ULL << 0)  // Host handles packets with partial checksum
#define VIRTIO_NET_F_GUEST_CSUM (1ULL << 1)  // Guest handles packets with partial checksum

static void (*netstack_rx_cb)(packet_buf_t *pkt) = NULL;
static uint64_t negotiated_features = 0;

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
    // Example: Negotiate checksum offload capability
    // In a real driver, we read device features and write negotiated features.
    // For this milestone, we pretend we successfully negotiated both TX and RX checksum offloading.
    negotiated_features |= VIRTIO_NET_F_CSUM;       // TX Offload Support
    negotiated_features |= VIRTIO_NET_F_GUEST_CSUM; // RX Offload Support
    return 0;
}

int virtio_net_start(void *device, void (*rx_callback)(packet_buf_t *)) {
    /* Configure rings, transition to DRIVER_OK, register RX callback */
    netstack_rx_cb = rx_callback;
    return 0;
}

int virtio_net_stop(void *device) {
    /* Halt queues, reset the device */
    netstack_rx_cb = NULL;
    return 0;
}

int virtio_net_tx(void *device, packet_buf_t *pkt) {
    /* Enqueue buffer into virtqueue for transmission.
       For the Phase 2 mock, we can just print or simulate sending. */

    // Check if TX offload was requested and negotiated
    if ((pkt->flags & PACKET_FLAG_TX_CSUM_REQ) && (negotiated_features & VIRTIO_NET_F_CSUM)) {
        // Setup virtio_net_hdr to request hardware checksum generation
        // e.g. hdr->flags = VIRTIO_NET_HDR_F_NEEDS_CSUM;
    } else {
        // Software generated checksum, no offload requested
    }

    // Note: hardware tx would claim ownership, unref upon completion
    packet_unref(pkt);
    return 0;
}

int virtio_net_poll(void *device) {
    /* Check ring status without interrupts and process pending packets */
    return 0;
}

// Declare memcpy locally or include string.h as allowed by environment
extern void *memcpy(void *dest, const void *src, size_t n);

/* Simulated RX for host testing/mocking in Phase 2 */
void virtio_net_mock_rx(const void *buffer, size_t length) {
    if (netstack_rx_cb) {
        packet_buf_t *pkt = packet_alloc();
        if (pkt) {
            // Check if packet fits in the buffer
            if (length <= pkt->tail_len) {
                memcpy(pkt->data, buffer, length);
                pkt->data_len = length;

                // Simulate reading VirtIO RX descriptor header.
                // If the hardware validated the checksum and we negotiated it:
                if (negotiated_features & VIRTIO_NET_F_GUEST_CSUM) {
                    // e.g. if (hdr->flags & VIRTIO_NET_HDR_F_DATA_VALID)
                    pkt->flags |= PACKET_FLAG_RX_CSUM_VALID;
                }

                netstack_rx_cb(pkt);
            } else {
                packet_free(pkt); // Drop packet, too large
            }
        }
    }
}
