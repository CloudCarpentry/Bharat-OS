#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <bharat/packet/packet.h>

#define NETDRV_MAX_DEVICES 8U
#define NETDRV_MAX_QUEUES 8U
#define NETDRV_MAC_LEN 6U

#define NETDRV_CAP_TX_CSUM      (1U << 0)
#define NETDRV_CAP_RX_CSUM      (1U << 1)
#define NETDRV_CAP_MULTICAST    (1U << 2)
#define NETDRV_CAP_PROMISC      (1U << 3)
#define NETDRV_CAP_POLL_FALLBACK (1U << 4)

typedef enum {
    NETDRV_STATE_PROBED = 0,
    NETDRV_STATE_INITIALIZED,
    NETDRV_STATE_STARTED,
    NETDRV_STATE_STOPPED,
    NETDRV_STATE_ERROR,
} netdrv_state_t;

typedef struct {
    uint64_t tx_packets;
    uint64_t rx_packets;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint64_t tx_drops;
    uint64_t rx_drops;
    uint64_t tx_errors;
    uint64_t rx_errors;
    uint64_t irq_count;
    uint64_t poll_count;
} netdrv_stats_t;

typedef struct {
    bool link_up;
    uint32_t mtu;
    uint32_t max_mtu;
    uint32_t min_mtu;
    uint32_t flags;
    uint8_t tx_queues;
    uint8_t rx_queues;
} netdrv_caps_t;

struct netdrv_device;
typedef struct netdrv_device netdrv_device_t;

typedef struct {
    int (*probe)(netdrv_device_t* dev, void* bus_device);
    int (*init)(netdrv_device_t* dev);
    int (*start)(netdrv_device_t* dev);
    int (*stop)(netdrv_device_t* dev);
    int (*tx)(netdrv_device_t* dev, packet_buf_t* pkt, uint8_t queue_id);
    int (*rx)(netdrv_device_t* dev, packet_buf_t** out_pkt, uint8_t queue_id);
    int (*poll)(netdrv_device_t* dev);
    int (*irq)(netdrv_device_t* dev, uint32_t irq_status);
    int (*set_mtu)(netdrv_device_t* dev, uint32_t mtu);
    int (*set_promisc)(netdrv_device_t* dev, bool enabled);
} netdrv_ops_t;

struct netdrv_device {
    const char* name;
    uint8_t device_id;
    uint8_t mac[NETDRV_MAC_LEN];
    const netdrv_ops_t* ops;
    netdrv_caps_t caps;
    netdrv_stats_t stats;
    netdrv_state_t state;
    bool promisc_enabled;
    void* bus_ctx;
    void* priv;
};

int netdrv_register(netdrv_device_t* dev);
int netdrv_unregister(netdrv_device_t* dev);
netdrv_device_t* netdrv_get(uint8_t device_id);
int netdrv_set_carrier(netdrv_device_t* dev, bool link_up);
int netdrv_set_mac(netdrv_device_t* dev, const uint8_t mac[NETDRV_MAC_LEN]);
int netdrv_set_mtu(netdrv_device_t* dev, uint32_t mtu);
int netdrv_submit_tx(netdrv_device_t* dev, packet_buf_t* pkt, uint8_t queue_id);
int netdrv_poll_rx(netdrv_device_t* dev, packet_buf_t** out_pkt, uint8_t queue_id);
int netdrv_handle_irq(netdrv_device_t* dev, uint32_t irq_status);
