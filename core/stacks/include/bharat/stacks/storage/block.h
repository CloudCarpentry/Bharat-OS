#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum {
    BLOCK_REQ_READ,
    BLOCK_REQ_WRITE,
    BLOCK_REQ_FLUSH
} block_req_type_t;

typedef struct {
    block_req_type_t type;
    uint64_t lba;
    uint32_t num_blocks;
    void* buffer; // Should be a DMA-safe capability buffer
    int status;
} block_request_t;

typedef struct {
    uint32_t device_id;
    uint32_t block_size;
    uint64_t total_blocks;
} block_device_info_t;

// API exposed to the filesystem daemon
int block_queue_request(uint32_t device_id, block_request_t* req);
int block_get_info(uint32_t device_id, block_device_info_t* info);
