#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <bharat/stacks/storage/block.h>
#include <bharat/stacks/storage/block_driver.h>

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

// Unfinished hardware path must return unsupported
int virtio_blk_submit_request(void* sg_list, uint32_t num_sgs) {
    (void)sg_list; (void)num_sgs;
    return -5; // -K_ERR_UNSUPPORTED (never return success for stubs)
}

static io_status_t virtio_blk_probe(void *driver_ctx) {
    (void)driver_ctx;
    return IO_STATUS_NOT_SUPPORTED;
}

static io_status_t virtio_blk_get_caps(void *driver_ctx, io_device_caps_t *out_caps) {
    (void)driver_ctx;
    if (!out_caps) return IO_STATUS_INVALID_ARGUMENT;

    // Provide some minimal fallback caps but indicate that we are a stub/scaffold
    out_caps->media_class = IO_MEDIA_SSD_SATA;
    out_caps->transport = IO_TRANSPORT_VIRTIO;
    out_caps->logical_block_size = 512;
    out_caps->physical_block_size = 512;
    out_caps->optimal_io_size = 512;
    out_caps->capacity_bytes = 512ULL * 1024ULL * 1024ULL; // 512 MB
    out_caps->max_transfer_bytes = 4096;
    out_caps->max_segments = 1;
    out_caps->max_hw_queues = 1;
    out_caps->max_queue_depth = 4;
    out_caps->read_only = false;
    out_caps->removable = false;
    out_caps->rotational = false;

    return IO_STATUS_OK;
}

static io_status_t virtio_blk_submit(void *channel, const io_request_t *request) {
    (void)channel; (void)request;
    return IO_STATUS_NOT_SUPPORTED;
}

static const block_driver_ops_t g_virtio_blk_ops = {
    .probe = virtio_blk_probe,
    .start = NULL,
    .stop = NULL,
    .get_caps = virtio_blk_get_caps,
    .open_channel = NULL,
    .close_channel = NULL,
    .submit = virtio_blk_submit,
    .submit_batch = NULL,
    .poll_completions = NULL,
    .cancel = NULL,
    .flush = NULL,
    .discard = NULL,
    .reset = NULL
};

static block_device_t g_virtio_blk_dev;
static bool g_registered = false;

void virtio_blk_init(void) {
    if (g_registered) return;

    g_virtio_blk_dev.id = 101U;
    g_virtio_blk_dev.name = "virtio-blk0";
    g_virtio_blk_dev.ops = &g_virtio_blk_ops;
    g_virtio_blk_dev.driver_context = NULL;
    virtio_blk_get_caps(NULL, &g_virtio_blk_dev.caps);
    g_virtio_blk_dev.state = IO_DEV_STATE_UNINITIALIZED; // Scaffold state
    g_virtio_blk_dev.role = IO_DEVICE_ROLE_DATA;

    block_driver_register(&g_virtio_blk_ops);
    block_device_register(&g_virtio_blk_dev);
    g_registered = true;
}
