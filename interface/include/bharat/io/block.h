#ifndef BHARAT_IO_BLOCK_H
#define BHARAT_IO_BLOCK_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t io_request_id_t;
typedef uint32_t io_device_id_t;
typedef uint32_t io_opcode_t;
typedef uint32_t capability_handle_t;

// Opcodes
#define IO_OP_READ    1U
#define IO_OP_WRITE   2U
#define IO_OP_FLUSH   3U
#define IO_OP_DISCARD 4U

// Request flags
#define IO_REQ_DIRECT             (1U << 0)
#define IO_REQ_BYPASS_CACHE       (1U << 1)
#define IO_REQ_FUA                (1U << 2)
#define IO_REQ_SYNC               (1U << 3)
#define IO_REQ_PREFETCH           (1U << 4)
#define IO_REQ_METADATA           (1U << 5)
#define IO_REQ_LOW_LATENCY        (1U << 6)
#define IO_REQ_BACKGROUND         (1U << 7)
#define IO_REQ_NO_MERGE           (1U << 8)

// Statuses
typedef enum {
    IO_STATUS_OK = 0,
    IO_STATUS_INVALID_ARGUMENT,
    IO_STATUS_NOT_SUPPORTED,
    IO_STATUS_NOT_READY,
    IO_STATUS_QUEUE_FULL,
    IO_STATUS_TIMEOUT,
    IO_STATUS_CANCELLED,
    IO_STATUS_IO_ERROR,
    IO_STATUS_DEVICE_REMOVED,
    IO_STATUS_PERMISSION_DENIED
} io_status_t;

typedef struct {
    uint64_t phys_addr;
    uint32_t length;
} io_sg_entry_t;

typedef struct {
    io_request_id_t id;
    io_device_id_t device_id;

    io_opcode_t opcode;
    uint64_t lba;
    uint32_t block_count;

    capability_handle_t buffer_cap;
    const io_sg_entry_t *segments;
    uint16_t segment_count;

    uint16_t priority;
    uint64_t deadline_ns;
    uint32_t flags;

    void *completion_cookie;
} io_request_t;

typedef struct {
    io_request_id_t id;
    io_device_id_t device_id;

    io_status_t status;
    uint32_t transferred_blocks;
    uint64_t completion_time_ns;

    void *completion_cookie;
} io_completion_t;

#endif /* BHARAT_IO_BLOCK_H */
