#ifndef BHARAT_FS_BLOCK_H
#define BHARAT_FS_BLOCK_H

#include <stdint.h>
#include <stddef.h>
#include "advanced/formal_verif.h"

/*
 * Generic block layer abstractions.
 * Provides a unified contract for asynchronous and synchronous I/O
 * (e.g., NVMe, virtio-blk, MMC, RAM disks).
 */

typedef enum {
    BLOCK_HINT_SYNC,
    BLOCK_HINT_DIRECT,
    BLOCK_HINT_BUFFERED,
    BLOCK_HINT_POLLING
} block_io_hint_t;

typedef enum {
    BLOCK_REQ_READ,
    BLOCK_REQ_WRITE,
    BLOCK_REQ_FLUSH,
    BLOCK_REQ_DISCARD
} block_req_type_t;

// Asynchronous block request structure
typedef struct block_request {
    block_req_type_t type;
    uint64_t sector_offset;
    uint32_t sector_count;
    void* buffer;           // Buffer for read/write

    // Hint for elevator/scheduler policies
    block_io_hint_t hint;

    // Capability gating access to this block operation
    capability_t* cap;

    // Completion callback (for async operations)
    void (*complete)(struct block_request* req, int status);

    // Status and next pointer for request queueing
    int status;
    struct block_request* next;
} block_request_t;

// A block device registered with the generic block layer
typedef struct block_device {
    char name[32];
    uint32_t sector_size;
    uint64_t total_sectors;

    // Device specific data (e.g. nvme namespace, virtio handle)
    void* driver_data;

    // Operations implemented by the underlying driver
    int (*submit_request)(struct block_device* dev, block_request_t* req);
    int (*flush)(struct block_device* dev);
    int (*get_info)(struct block_device* dev, void* info_out);

} block_device_t;

// Register a block device with the generic block layer
int block_device_register(block_device_t* dev, capability_t* cap);

// Allocate and submit a request to a block device
int block_request_submit(block_device_t* dev, block_request_t* req, capability_t* cap);

#endif // BHARAT_FS_BLOCK_H