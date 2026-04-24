#ifndef BHARAT_NET_NETDEV_H
#define BHARAT_NET_NETDEV_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Network Device Interface
 *
 * Provides an abstraction for NICs exposed from the Kernel Device/Driver
 * framework to the user-space network stack service.
 */

#define MAC_ADDR_LEN 6

typedef struct netdev_stats {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_errors;
    uint64_t tx_errors;
    uint64_t rx_dropped;
    uint64_t tx_dropped;
} netdev_stats_t;

/* A packet buffer abstraction (mbuf/skb equivalent) */
typedef struct netbuf {
    void* data;          /* Pointer to payload start */
    uint32_t len;        /* Length of packet */
    uint32_t head_room;  /* Space reserved before payload */
    uint32_t tail_room;  /* Space reserved after payload */
    struct netbuf* next; /* Linked list of buffers for scatter/gather */
    void* _priv;         /* Backend specific context (e.g. physical addr) */
} netbuf_t;

typedef struct netdev {
    const char* name;          /* Interface name, e.g. "eth0" */
    uint8_t mac[MAC_ADDR_LEN]; /* Hardware MAC address */
    uint32_t mtu;              /* Maximum Transmission Unit */
    uint32_t flags;            /* Interface flags (UP, RUNNING, PROMISC) */

    netdev_stats_t stats;

    /* Device operations implemented by specific NIC driver */
    int (*init)(struct netdev* dev);
    int (*open)(struct netdev* dev);
    int (*close)(struct netdev* dev);
    int (*xmit)(struct netdev* dev, netbuf_t* buf);

    /* Private data pointer for driver context (e.g., PCIe MMIO regs, zero-copy URPC rings) */
    void* priv;
} netdev_t;

/* Register a new network device */
int netdev_register(netdev_t* dev);

/* Handle an incoming packet (called by driver IRQ or poll thread) */
int netdev_receive(netdev_t* dev, netbuf_t* buf);

#endif /* BHARAT_NET_NETDEV_H */
