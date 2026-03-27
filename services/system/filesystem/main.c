#include <stdint.h>
#include <stddef.h>
#include "urpc.h"
#include "bharat/stacks/storage/block.h"

// Reference happy path execution
extern int virtio_blk_submit_request(void* sg_list, uint32_t num_sgs);
extern void virtio_blk_init(void);

// Simulated capability IPC server handler
void handle_fs_urpc_request(void* msg) {
    (void)msg; // Suppress unused
    // App -> Service (VFS)
    // Here VFS would decode `msg`, validate capability, check mount, and open/read file.

    // Service -> Stack (Block)
    block_request_t breq = {
        .type = BLOCK_REQ_READ,
        .lba = 0,
        .num_blocks = 1,
        .buffer = NULL, // Provide simulated DMA buffer here
        .status = -1
    };

    block_queue_request(0, &breq); // Stacks block abstraction
}

int main(void) {
    // 1. Initialize drivers
    virtio_blk_init();

    // 2. Wait for incoming capability requests
    // (Simulate an incoming uRPC request loop)

    return 0;
}
