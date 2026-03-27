#include "bharat/stacks/storage/block.h"

// Forward declare the driver function as an example
extern int virtio_blk_submit_request(void* sg_list, uint32_t num_sgs);

int block_queue_request(uint32_t device_id, block_request_t* req) {
    // Stub: Serialize request and send to driver via uRPC
    if (!req) return -1;

    if (device_id == 0) { // Assume 0 is virtio-blk
        // We simulate sending an SG list to the driver
        void* dummy_sg = req->buffer;
        req->status = virtio_blk_submit_request(dummy_sg, 1);
    } else {
        req->status = 0; // Success stub for generic
    }

    return 0;
}

int block_get_info(uint32_t device_id, block_device_info_t* info) {
    if (!info) return -1;
    info->device_id = device_id;
    info->block_size = 512;
    info->total_blocks = 1024 * 1024; // 512 MB stub
    return 0;
}
