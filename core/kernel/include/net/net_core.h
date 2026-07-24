#ifndef BHARAT_NET_CORE_H
#define BHARAT_NET_CORE_H

#include <stdint.h>
#include <stddef.h>
#include "netdev.h"

/*
 * Bharat-OS Networking Core Primitives
 *
 * Defines the essential contracts for:
 * 1. Shared-memory ring buffers between drivers and the network data plane.
 * 2. Event notifications for queues and link state.
 * 3. Minimal structural metadata around `netbuf_t`.
 */

/*
 * Event types for network queue notifications and link state changes.
 */
typedef enum {
    NET_EVENT_NONE = 0,
    NET_EVENT_LINK_UP,
    NET_EVENT_LINK_DOWN,
    NET_EVENT_RX_AVAILABLE,
    NET_EVENT_TX_COMPLETE,
    NET_EVENT_QUEUE_ERROR
} net_event_type_t;

/*
 * Ring buffer abstraction for zero-copy driver ↔ net dataplane communication.
 */
typedef struct net_ring {
    netbuf_t** descriptors; /* Array of pointers to netbuf_t descriptors */
    uint32_t size;          /* Total number of descriptor slots (power of 2) */
    uint32_t head;          /* Producer index */
    uint32_t tail;          /* Consumer index */
    uint32_t flags;         /* Ring state flags (e.g. running, stalled) */
    void* _priv;            /* Backend specific context */
} net_ring_t;

/*
 * Interface state values.
 */
typedef enum {
    NET_LINK_DOWN = 0,
    NET_LINK_UP   = 1
} net_link_state_t;

#endif /* BHARAT_NET_CORE_H */
