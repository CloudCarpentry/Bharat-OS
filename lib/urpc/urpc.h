#ifndef BHARAT_URPC_H
#define BHARAT_URPC_H

#include <stdint.h>
#include <stddef.h>

/*
 * Universal RPC (URPC) Library
 *
 * Provides a high-throughput lockless ring-buffer for asynchronous messaging
 * over shared memory.
 */

#define URPC_SUCCESS 0
#define URPC_ERR_FULL -1
#define URPC_ERR_EMPTY -2
#define URPC_ERR_INVALID -3

typedef struct {
    uint32_t type;
    uint32_t length;
    uint8_t payload[56]; // Ensure a 64-byte message block
} urpc_msg_t;

typedef struct {
    uint32_t head;       // Writer index
    uint32_t tail;       // Reader index
    uint32_t capacity;   // Total capacity (must be power of 2)
    uint32_t mask;       // capacity - 1
    urpc_msg_t* buffer;  // Pointer to the start of the message array
} urpc_channel_t;

/* Initialize a URPC channel on a pre-allocated shared memory region. */
int urpc_init_channel(urpc_channel_t* channel, void* shared_mem, uint32_t size_bytes);

/* Send a message locklessly. */
int urpc_send(urpc_channel_t* channel, const urpc_msg_t* msg);

/* Receive a message locklessly. */
int urpc_receive(urpc_channel_t* channel, urpc_msg_t* msg);

#endif // BHARAT_URPC_H
