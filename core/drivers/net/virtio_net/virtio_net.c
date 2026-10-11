#include <stddef.h>
#include <stdint.h>

#include <bharat/packet/packet.h>
#include "drivers/net/net_driver.h"
#include "../../virtio/pci/virtio_pci.h"

#define VIRTIO_NET_F_CSUM        (1ULL << 0)
#define VIRTIO_NET_F_GUEST_CSUM  (1ULL << 1)

#define VIRTIO_NET_RXQ_LEN 64U

typedef struct {
    packet_buf_t* rx_ring[VIRTIO_NET_RXQ_LEN];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t rx_count;
    uint64_t negotiated_features;
    bool started;

    bh_virtio_pci_device_t vpci;
    bool is_real_pci;

    bh_virtqueue_t rx_vq;
    bh_virtqueue_t tx_vq;

    _Alignas(4096) bh_virtq_desc_t rx_desc[VIRTIO_RING_SIZE];
    _Alignas(4096) bh_virtq_avail_t rx_avail;
    _Alignas(4096) bh_virtq_used_t rx_used;

    _Alignas(4096) bh_virtq_desc_t tx_desc[VIRTIO_RING_SIZE];
    _Alignas(4096) bh_virtq_avail_t tx_avail;
    _Alignas(4096) bh_virtq_used_t tx_used;

    packet_buf_t *rx_buffers[VIRTIO_RING_SIZE];
    packet_buf_t *tx_buffers[VIRTIO_RING_SIZE];
} virtio_net_priv_t;

static void (*netstack_rx_cb)(packet_buf_t* pkt);
static virtio_net_priv_t g_vnet_priv;
static netdrv_device_t g_vnet_device;

static int virtio_drv_probe(netdrv_device_t* dev, void* bus_device) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;
    dev->bus_ctx = bus_device;

    pci_device_t *pci = (pci_device_t *)bus_device;
    if (pci) {
        int rc = bh_virtio_pci_probe(&priv->vpci, pci);
        if (rc == 0) {
            priv->is_real_pci = true;
        }
    }

    dev->state = NETDRV_STATE_PROBED;
    return 0;
}

static int virtio_drv_init(netdrv_device_t* dev) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;

    priv->rx_head = 0;
    priv->rx_tail = 0;
    priv->rx_count = 0;
    priv->started = false;
    priv->negotiated_features = VIRTIO_NET_F_CSUM | VIRTIO_NET_F_GUEST_CSUM;

    if (priv->is_real_pci) {
        bh_virtio_pci_negotiate_features(&priv->vpci, priv->negotiated_features, &priv->negotiated_features);
    }

    dev->state = NETDRV_STATE_INITIALIZED;
    return 0;
}

static int virtio_drv_start(netdrv_device_t* dev) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;

    if (priv->is_real_pci) {
        // Setup RX (Queue 0)
        int rc = bh_virtio_pci_setup_queue(&priv->vpci, 0, &priv->rx_vq, priv->rx_desc, &priv->rx_avail, &priv->rx_used);
        if (rc != 0) return rc;

        // Setup TX (Queue 1)
        rc = bh_virtio_pci_setup_queue(&priv->vpci, 1, &priv->tx_vq, priv->tx_desc, &priv->tx_avail, &priv->tx_used);
        if (rc != 0) return rc;

        for (int i = 0; i < VIRTIO_RING_SIZE; i++) {
            priv->rx_buffers[i] = NULL;
            priv->tx_buffers[i] = NULL;
        }

        // Fill RX ring
        for (int i = 0; i < VIRTIO_RING_SIZE; i++) {
            packet_buf_t *pkt = packet_alloc();
            if (pkt) {
                uint16_t desc_idx;
                if (bh_virtqueue_add_rx_buffer(&priv->rx_vq, pkt->data, pkt->total_len, &desc_idx) == 0) {
                    priv->rx_buffers[desc_idx] = pkt;
                } else {
                    packet_free(pkt);
                }
            }
        }

        bh_virtio_pci_notify_queue(&priv->vpci, 0, &priv->rx_vq);

        rc = bh_virtio_pci_start_device(&priv->vpci);
        if (rc != 0) return rc;
    }

    priv->started = true;
    dev->state = NETDRV_STATE_STARTED;
    return netdrv_set_carrier(dev, true);
}

static int virtio_drv_stop(netdrv_device_t* dev) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;

    priv->started = false;
    dev->state = NETDRV_STATE_STOPPED;
    return netdrv_set_carrier(dev, false);
}

static int virtio_drv_tx(netdrv_device_t* dev, packet_buf_t* pkt, uint8_t queue_id) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;

    (void)queue_id;
    if (!priv->started) {
        return -1;
    }

    if ((pkt->flags & PACKET_FLAG_TX_CSUM_REQ) &&
        (priv->negotiated_features & VIRTIO_NET_F_CSUM) == 0) {
        dev->stats.tx_errors++;
        packet_unref(pkt);
        return -1;
    }

    if (priv->is_real_pci) {
        uint16_t desc_idx;
        // The virtqueue_add_tx_buffer handles passing the buffer address.
        // It requires the virtqueue headers to be at the beginning of the buffer for virtio-net,
        // but for a stub/first end-to-end path, let's just send the data directly.
        // QEMU requires a virtio_net_hdr (10 bytes or 12 bytes).
        // Since we didn't negotiate any special header lengths, we must prepend a 10-byte header.
        // Let's use the headroom we have in packet_buf_t.
        if (pkt->head_len >= 10) {
            pkt->head_len -= 10;
            pkt->data -= 10;
            pkt->data_len += 10;
            __builtin_memset(pkt->data, 0, 10); // Empty virtio_net_hdr
        } else {
            dev->stats.tx_errors++;
            packet_unref(pkt);
            return -1;
        }

        if (bh_virtqueue_add_tx_buffer(&priv->tx_vq, pkt->data, pkt->data_len, &desc_idx) == 0) {
            priv->tx_buffers[desc_idx] = pkt;
            bh_virtio_pci_notify_queue(&priv->vpci, 1, &priv->tx_vq);
            return 0;
        }
        dev->stats.tx_drops++;
        packet_unref(pkt);
        return -1;
    } else {
        packet_unref(pkt);
        return 0;
    }
}

static int virtio_drv_rx(netdrv_device_t* dev, packet_buf_t** out_pkt, uint8_t queue_id) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;
    packet_buf_t* pkt = NULL;

    (void)dev;
    (void)queue_id;
    if (!out_pkt) {
        return -1;
    }

    if (priv->is_real_pci) {
        uint16_t desc_idx;
        uint32_t len;
        if (bh_virtqueue_poll_used(&priv->rx_vq, &desc_idx, &len)) {
            pkt = priv->rx_buffers[desc_idx];
            priv->rx_buffers[desc_idx] = NULL;
            bh_virtqueue_free_descriptor(&priv->rx_vq, desc_idx);

            if (pkt && len >= 10) {
                // Strip virtio_net_hdr
                pkt->data += 10;
                pkt->head_len += 10;
                pkt->data_len = len - 10;
                *out_pkt = pkt;

                // Replenish rx ring immediately
                packet_buf_t *new_pkt = packet_alloc();
                if (new_pkt) {
                    uint16_t new_desc;
                    if (bh_virtqueue_add_rx_buffer(&priv->rx_vq, new_pkt->data, new_pkt->total_len, &new_desc) == 0) {
                        priv->rx_buffers[new_desc] = new_pkt;
                        bh_virtio_pci_notify_queue(&priv->vpci, 0, &priv->rx_vq);
                    } else {
                        packet_free(new_pkt);
                    }
                }
                return 0;
            } else if (pkt) {
                packet_free(pkt);
            }
        }
        return -1;
    }

    if (priv->rx_count == 0) {
        return -1;
    }

    pkt = priv->rx_ring[priv->rx_head];
    priv->rx_ring[priv->rx_head] = 0;
    priv->rx_head = (uint16_t)((priv->rx_head + 1U) % VIRTIO_NET_RXQ_LEN);
    priv->rx_count--;
    *out_pkt = pkt;
    return 0;
}

static int virtio_drv_poll(netdrv_device_t* dev) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;
    packet_buf_t* pkt = 0;

    if (!dev) {
        return -1;
    }

    dev->stats.poll_count++;

    // Free completed TX packets
    if (priv->is_real_pci) {
        uint16_t desc_idx;
        uint32_t len;
        while (bh_virtqueue_poll_used(&priv->tx_vq, &desc_idx, &len)) {
            packet_buf_t *tx_pkt = priv->tx_buffers[desc_idx];
            priv->tx_buffers[desc_idx] = NULL;
            bh_virtqueue_free_descriptor(&priv->tx_vq, desc_idx);
            if (tx_pkt) {
                packet_unref(tx_pkt);
            }
        }
    }

    // Process RX
    while (netdrv_poll_rx(dev, &pkt, 0) == 0) {
        if (netstack_rx_cb) {
            netstack_rx_cb(pkt);
        } else {
            packet_free(pkt);
        }
        pkt = 0;
    }
    return 0;
}

static int virtio_drv_irq(netdrv_device_t* dev, uint32_t irq_status) {
    (void)irq_status;
    return virtio_drv_poll(dev);
}

static int virtio_drv_set_mtu(netdrv_device_t* dev, uint32_t mtu) {
    (void)dev;
    if (mtu < 576 || mtu > 9000) {
        return -1;
    }
    return 0;
}

static int virtio_drv_set_promisc(netdrv_device_t* dev, bool enabled) {
    dev->promisc_enabled = enabled;
    return 0;
}

static const netdrv_ops_t g_vnet_ops = {
    .probe = virtio_drv_probe,
    .init = virtio_drv_init,
    .start = virtio_drv_start,
    .stop = virtio_drv_stop,
    .tx = virtio_drv_tx,
    .rx = virtio_drv_rx,
    .poll = virtio_drv_poll,
    .irq = virtio_drv_irq,
    .set_mtu = virtio_drv_set_mtu,
    .set_promisc = virtio_drv_set_promisc,
};

int virtio_net_init(void) {
    static const uint8_t default_mac[NETDRV_MAC_LEN] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

    g_vnet_device.name = "virtio-net0";
    g_vnet_device.device_id = 0;
    g_vnet_device.ops = &g_vnet_ops;
    g_vnet_device.priv = &g_vnet_priv;
    g_vnet_device.caps.link_up = false;
    g_vnet_device.caps.mtu = 1500;
    g_vnet_device.caps.min_mtu = 576;
    g_vnet_device.caps.max_mtu = 9000;
    g_vnet_device.caps.flags = NETDRV_CAP_TX_CSUM | NETDRV_CAP_RX_CSUM |
                               NETDRV_CAP_MULTICAST | NETDRV_CAP_PROMISC |
                               NETDRV_CAP_POLL_FALLBACK;
    g_vnet_device.caps.tx_queues = 1;
    g_vnet_device.caps.rx_queues = 1;

    if (netdrv_register(&g_vnet_device) != 0) {
        return -1;
    }

    return netdrv_set_mac(&g_vnet_device, default_mac);
}

int virtio_net_probe(void* device) {
    if (!g_vnet_device.ops || !g_vnet_device.ops->probe) {
        return -1;
    }
    return g_vnet_device.ops->probe(&g_vnet_device, device);
}

int virtio_net_bind(void* device) {
    (void)device;
    if (!g_vnet_device.ops || !g_vnet_device.ops->init) {
        return -1;
    }
    return g_vnet_device.ops->init(&g_vnet_device);
}

int virtio_net_start(void* device, void (*rx_callback)(packet_buf_t*)) {
    (void)device;
    netstack_rx_cb = rx_callback;
    if (!g_vnet_device.ops || !g_vnet_device.ops->start) {
        return -1;
    }
    return g_vnet_device.ops->start(&g_vnet_device);
}

int virtio_net_stop(void* device) {
    (void)device;
    netstack_rx_cb = 0;
    if (!g_vnet_device.ops || !g_vnet_device.ops->stop) {
        return -1;
    }
    return g_vnet_device.ops->stop(&g_vnet_device);
}

int virtio_net_tx(void* device, packet_buf_t* pkt) {
    (void)device;
    return netdrv_submit_tx(&g_vnet_device, pkt, 0);
}

int virtio_net_poll(void* device) {
    (void)device;
    if (!g_vnet_device.ops || !g_vnet_device.ops->poll) {
        return -1;
    }
    return g_vnet_device.ops->poll(&g_vnet_device);
}

void virtio_net_mock_rx(const void* buffer, size_t length) {
    packet_buf_t* pkt;

    if (length == 0 || g_vnet_priv.rx_count >= VIRTIO_NET_RXQ_LEN) {
        g_vnet_device.stats.rx_drops++;
        return;
    }

    pkt = packet_alloc();
    if (!pkt) {
        g_vnet_device.stats.rx_drops++;
        return;
    }

    if (length > pkt->tail_len) {
        packet_free(pkt);
        g_vnet_device.stats.rx_drops++;
        return;
    }

    __builtin_memcpy(pkt->data, buffer, length);
    pkt->data_len = (uint16_t)length;

    if (g_vnet_priv.negotiated_features & VIRTIO_NET_F_GUEST_CSUM) {
        pkt->flags |= PACKET_FLAG_RX_CSUM_VALID;
    }

    g_vnet_priv.rx_ring[g_vnet_priv.rx_tail] = pkt;
    g_vnet_priv.rx_tail = (uint16_t)((g_vnet_priv.rx_tail + 1U) % VIRTIO_NET_RXQ_LEN);
    g_vnet_priv.rx_count++;
}
