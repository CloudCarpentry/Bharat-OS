#include <stdint.h>
#include <stddef.h>

/*
 * virtio_blk.c
 *
 * Hardware block driver implementation.
 *
 * Responsibilities:
 * - MMIO / PCI register interaction for VirtIO capabilities.
 * - Virtqueue setup and management.
 * - DMA descriptor chaining.
 * - Handling block requests from stacks/storage/block.
 *
 * Note: This driver MUST NOT include any VFS or POSIX headers.
 */

typedef struct {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
} virtio_blk_outhdr_t;

typedef struct {
    uint8_t status;
} virtio_blk_inhdr_t;

// Example stub to simulate handling an incoming block request
int virtio_blk_submit_request(void* sg_list, uint32_t num_sgs) {
    if (!sg_list || num_sgs == 0) return -1;
    // 1. Write virtio_blk_outhdr to queue
    // 2. Chain SG list buffers (DMA memory)
    // 3. Chain virtio_blk_inhdr
    // 4. Ring MMIO doorbell
    return 0; // Simulated success
}

void virtio_blk_init(void) {
    // 1. Probe PCI device
    // 2. Negotiate features (e.g., VIRTIO_BLK_F_SIZE_MAX)
    // 3. Initialize virtqueues
}
