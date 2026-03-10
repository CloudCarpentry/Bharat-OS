#include "fs/block.h"

#define MAX_BLOCK_DEVICES 8

static block_device_t* g_block_devices[MAX_BLOCK_DEVICES];
static size_t g_block_device_count = 0;

int block_device_register(block_device_t* dev, capability_t* cap) {
    if (!dev || !dev->submit_request || !cap) {
        return -1;
    }

    // TODO: Verify capability allows block device registration

    if (g_block_device_count >= MAX_BLOCK_DEVICES) {
        return -1;
    }

    g_block_devices[g_block_device_count++] = dev;
    return 0;
}

int block_request_submit(block_device_t* dev, block_request_t* req, capability_t* cap) {
    if (!dev || !req || !dev->submit_request || !cap) {
        return -1;
    }

    // TODO: Verify capability matches device

    // Enqueue request or submit to underlying driver
    return dev->submit_request(dev, req);
}
