#ifndef NETSTACK_NETBUF_H
#define NETSTACK_NETBUF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <bharat/runtime/freestanding_string.h>

/* A basic network buffer for Phase 2, similar in concept to sk_buff or pbuf,
   but strictly self-contained within netstack for now. */

#define NETBUF_MAX_SIZE 2048
#define NETBUF_DEFAULT_HEADROOM 256

#define NETBUF_F_RX_IP_CSUM_OK       (1u << 0) // Hardware has validated IP header checksum
#define NETBUF_F_RX_L4_CSUM_OK       (1u << 1) // Hardware has validated L4 (TCP/UDP) checksum
#define NETBUF_F_RX_IP_CSUM_BAD      (1u << 2) // Hardware detected invalid IP checksum
#define NETBUF_F_RX_L4_CSUM_BAD      (1u << 3) // Hardware detected invalid L4 checksum

#define NETBUF_F_TX_IP_CSUM_PARTIAL  (1u << 4) // Request hardware to generate IP header checksum
#define NETBUF_F_TX_L4_CSUM_PARTIAL  (1u << 5) // Request hardware to generate L4 checksum
#define NETBUF_F_TX_IP_CSUM_DONE     (1u << 6) // Software has generated IP checksum
#define NETBUF_F_TX_L4_CSUM_DONE     (1u << 7) // Software has generated L4 checksum

typedef struct {
    uint8_t buffer[NETBUF_MAX_SIZE];
    uint16_t head;      // Offset to start of data
    uint16_t tail;      // Offset to end of data
    uint16_t size;      // Total capacity
    uint32_t flags;     // Stack-level packet metadata and offload flags
} netbuf_t;

// Initialize a netbuf with default headroom
void netbuf_init(netbuf_t *nb);

// Get a pointer to the current data
uint8_t *netbuf_data(netbuf_t *nb);

// Get the current length of the data
uint16_t netbuf_len(netbuf_t *nb);

// Push data to the front (e.g. adding headers). Returns pointer to new start of data, or NULL on failure.
uint8_t *netbuf_push(netbuf_t *nb, uint16_t len);

// Pull data from the front (e.g. parsing headers). Returns pointer to old start of data, or NULL on failure.
uint8_t *netbuf_pull(netbuf_t *nb, uint16_t len);

// Append data to the end (e.g. adding payload). Returns pointer to start of appended region, or NULL on failure.
uint8_t *netbuf_put(netbuf_t *nb, uint16_t len);

#endif // NETSTACK_NETBUF_H
