#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file packet.h
 * @brief Common packet abstractions for Bharat-OS.
 *
 * Defines the buffer descriptor structure, ownership, headroom, tailroom,
 * and future flags for checksum offloads.
 */

// Packet flags
#define PACKET_FLAG_RX_CSUM_VALID   (1 << 0)
#define PACKET_FLAG_TX_CSUM_REQ     (1 << 1)
#define PACKET_FLAG_BCAST           (1 << 2)
#define PACKET_FLAG_MCAST           (1 << 3)
#define PACKET_FLAG_VLAN            (1 << 4)
#define PACKET_FLAG_DMA_MAPPED      (1 << 5)

/**
 * @brief A packet buffer descriptor.
 *
 * Describes a region of memory containing network data.
 * The memory is typically managed by a slab/pool allocator, and ownership
 * is passed between domains (NIC drivers -> netstack -> user apps).
 */
typedef struct packet_buf_s {
    uint8_t *data;         // Pointer to the start of the buffer
    uint16_t head_len;     // Headroom size (for adding headers)
    uint16_t tail_len;     // Tailroom size (for adding trailers)
    uint32_t total_len;    // Total allocated size of the buffer
    uint32_t data_len;     // Length of valid data within the buffer
    uint32_t flags;        // Packet flags (checksum, offloads, etc.)

    uint32_t refcount;     // Reference count
    struct packet_buf_s *next; // For fragment chaining

    // Placeholder for ownership tracking or reference counting
    void *owner_ctx;
} packet_buf_t;

// Basic initialization contract
void libpacket_init(void);

// Allocate a packet buffer from the pool
packet_buf_t *packet_alloc(void);

// Free or unref a packet buffer
void packet_free(packet_buf_t *pkt);

// Increment refcount
void packet_ref(packet_buf_t *pkt);

// Decrement refcount, free if 0
void packet_unref(packet_buf_t *pkt);
