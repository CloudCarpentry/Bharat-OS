#include "bharat/stacks/storage/block.h"

int block_queue_request(uint32_t device_id, block_request_t* req) {
    // Stub: Serialize request and send to driver via uRPC
    if (!req) return -1;
    req->status = 0; // Success stub
    return 0;
}

int block_get_info(uint32_t device_id, block_device_info_t* info) {
    if (!info) return -1;
    info->device_id = device_id;
    info->block_size = 512;
    info->total_blocks = 1024 * 1024; // 512 MB stub
    return 0;
}
