#include "drivers/net/net_driver.h"

static netdrv_device_t* g_netdrv_devices[NETDRV_MAX_DEVICES];

int netdrv_register(netdrv_device_t* dev) {
    if (!dev || !dev->ops || !dev->name) {
        return -1;
    }
    if (dev->device_id >= NETDRV_MAX_DEVICES) {
        return -1;
    }
    if (g_netdrv_devices[dev->device_id]) {
        return -1;
    }
    if (dev->caps.tx_queues == 0 || dev->caps.tx_queues > NETDRV_MAX_QUEUES) {
        return -1;
    }
    if (dev->caps.rx_queues == 0 || dev->caps.rx_queues > NETDRV_MAX_QUEUES) {
        return -1;
    }
    if (dev->caps.min_mtu == 0 || dev->caps.max_mtu < dev->caps.min_mtu) {
        return -1;
    }
    if (dev->caps.mtu < dev->caps.min_mtu || dev->caps.mtu > dev->caps.max_mtu) {
        return -1;
    }

    dev->state = NETDRV_STATE_PROBED;
    g_netdrv_devices[dev->device_id] = dev;
    return 0;
}

int netdrv_unregister(netdrv_device_t* dev) {
    if (!dev || dev->device_id >= NETDRV_MAX_DEVICES) {
        return -1;
    }
    if (g_netdrv_devices[dev->device_id] != dev) {
        return -1;
    }
    g_netdrv_devices[dev->device_id] = 0;
    return 0;
}

netdrv_device_t* netdrv_get(uint8_t device_id) {
    if (device_id >= NETDRV_MAX_DEVICES) {
        return 0;
    }
    return g_netdrv_devices[device_id];
}

int netdrv_set_carrier(netdrv_device_t* dev, bool link_up) {
    if (!dev) {
        return -1;
    }
    dev->caps.link_up = link_up;
    return 0;
}

int netdrv_set_mac(netdrv_device_t* dev, const uint8_t mac[NETDRV_MAC_LEN]) {
    size_t i;

    if (!dev || !mac) {
        return -1;
    }
    for (i = 0; i < NETDRV_MAC_LEN; ++i) {
        dev->mac[i] = mac[i];
    }
    return 0;
}

int netdrv_set_mtu(netdrv_device_t* dev, uint32_t mtu) {
    if (!dev) {
        return -1;
    }
    if (mtu < dev->caps.min_mtu || mtu > dev->caps.max_mtu) {
        return -1;
    }
    if (dev->ops->set_mtu) {
        if (dev->ops->set_mtu(dev, mtu) != 0) {
            return -1;
        }
    }
    dev->caps.mtu = mtu;
    return 0;
}

int netdrv_submit_tx(netdrv_device_t* dev, packet_buf_t* pkt, uint8_t queue_id) {
    if (!dev || !pkt || !dev->ops || !dev->ops->tx) {
        return -1;
    }
    if (queue_id >= dev->caps.tx_queues) {
        return -1;
    }
    if (dev->state != NETDRV_STATE_STARTED || !dev->caps.link_up) {
        dev->stats.tx_drops++;
        return -1;
    }

    if (dev->ops->tx(dev, pkt, queue_id) != 0) {
        dev->stats.tx_errors++;
        return -1;
    }

    dev->stats.tx_packets++;
    dev->stats.tx_bytes += pkt->data_len;
    return 0;
}

int netdrv_poll_rx(netdrv_device_t* dev, packet_buf_t** out_pkt, uint8_t queue_id) {
    if (!dev || !out_pkt || !dev->ops || !dev->ops->rx) {
        return -1;
    }
    if (queue_id >= dev->caps.rx_queues) {
        return -1;
    }

    if (dev->ops->rx(dev, out_pkt, queue_id) != 0) {
        return -1;
    }
    if (!*out_pkt) {
        return -1;
    }

    dev->stats.rx_packets++;
    dev->stats.rx_bytes += (*out_pkt)->data_len;
    return 0;
}

int netdrv_handle_irq(netdrv_device_t* dev, uint32_t irq_status) {
    if (!dev || !dev->ops || !dev->ops->irq) {
        return -1;
    }
    dev->stats.irq_count++;
    return dev->ops->irq(dev, irq_status);
}
