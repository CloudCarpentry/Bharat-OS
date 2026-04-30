#include "fs/block.h"
#include "kernel/status.h"

// PHASE1_COMPAT_SHIM: Block device logic moved to stacks/storage/.

int block_device_register(block_device_t* dev, capability_t* cap) {
    (void)dev;
    (void)cap;
    return K_ERR_REQUIRES_FS_SERVICE;
}

int block_request_submit(block_device_t* dev, block_request_t* req, capability_t* cap) {
    (void)dev;
    (void)req;
    (void)cap;
    return K_ERR_REQUIRES_FS_SERVICE;
}
