#include <stddef.h>
#include <stdint.h>

#include <bharat/packet/packet.h>
#include "drivers/net/net_driver.h"

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
} virtio_net_priv_t;

static void (*netstack_rx_cb)(packet_buf_t* pkt);
static virtio_net_priv_t g_vnet_priv;
static netdrv_device_t g_vnet_device;

static int virtio_drv_probe(netdrv_device_t* dev, void* bus_device) {
    dev->bus_ctx = bus_device;
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

    dev->state = NETDRV_STATE_INITIALIZED;
    return 0;
}

static int virtio_drv_start(netdrv_device_t* dev) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;

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

    packet_unref(pkt);
    return 0;
}

static int virtio_drv_rx(netdrv_device_t* dev, packet_buf_t** out_pkt, uint8_t queue_id) {
    virtio_net_priv_t* priv = (virtio_net_priv_t*)dev->priv;
    packet_buf_t* pkt;

    (void)dev;
    (void)queue_id;
    if (!out_pkt || priv->rx_count == 0) {
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
    packet_buf_t* pkt = 0;

    if (!dev) {
        return -1;
    }

    dev->stats.poll_count++;
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
