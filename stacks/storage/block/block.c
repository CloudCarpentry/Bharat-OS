#include "bharat/stacks/storage/block.h"

// Forward declare the driver function as an example
extern int virtio_blk_submit_request(void* sg_list, uint32_t num_sgs);

int block_queue_request(uint32_t device_id, block_request_t* req) {
    // Stub: Serialize request and send to driver via uRPC
    if (!req) return -1;

    if (device_id != 0) {
        req->status = -1;
        return -1;
    }

    if (req->type == BLOCK_REQ_FLUSH) {
        // Queue drain / barrier semantic placeholder for virtio device 0.
        req->status = 0;
        return 0;
    }

    // We simulate sending an SG list to the driver
    void* dummy_sg = req->buffer;
    req->status = virtio_blk_submit_request(dummy_sg, 1);

    return req->status;
}

int block_get_info(uint32_t device_id, block_device_info_t* info) {
    if (!info) return -1;
    if (device_id != 0) return -1;

    info->device_id = device_id;
    info->block_size = 512;
    info->total_blocks = 1024 * 1024; // 512 MB stub
    return 0;
}
