#include <bharat/stacks/storage/block.h>
#include <bharat/io_config.h>
#include <lib/base/string.h>

#define MAX_REG_DRIVERS 16U
#define MAX_REG_DEVICES 32U

static const block_driver_ops_t *g_drivers[MAX_REG_DRIVERS];
static uint32_t g_driver_count = 0U;

static block_device_t *g_devices[MAX_REG_DEVICES];
static uint32_t g_device_count = 0U;

io_status_t block_driver_register(const block_driver_ops_t *ops) {
    if (!ops) return IO_STATUS_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < MAX_REG_DRIVERS; ++i) {
        if (g_drivers[i] == ops) {
            return IO_STATUS_OK; // Already registered
        }
    }

    for (uint32_t i = 0; i < MAX_REG_DRIVERS; ++i) {
        if (!g_drivers[i]) {
            g_drivers[i] = ops;
            g_driver_count++;
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_QUEUE_FULL;
}

io_status_t block_driver_unregister(const block_driver_ops_t *ops) {
    if (!ops) return IO_STATUS_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < MAX_REG_DRIVERS; ++i) {
        if (g_drivers[i] == ops) {
            g_drivers[i] = NULL;
            if (g_driver_count > 0U) g_driver_count--;
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_INVALID_ARGUMENT;
}

io_status_t block_device_register(block_device_t *device) {
    if (!device) return IO_STATUS_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < MAX_REG_DEVICES; ++i) {
        if (g_devices[i] && g_devices[i]->id == device->id) {
            return IO_STATUS_INVALID_ARGUMENT; // Duplicate ID
        }
    }

    for (uint32_t i = 0; i < MAX_REG_DEVICES; ++i) {
        if (!g_devices[i]) {
            g_devices[i] = device;
            g_device_count++;
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_QUEUE_FULL;
}

io_status_t block_device_unregister(io_device_id_t device_id) {
    for (uint32_t i = 0; i < MAX_REG_DEVICES; ++i) {
        if (g_devices[i] && g_devices[i]->id == device_id) {
            g_devices[i]->state = IO_DEV_STATE_REMOVED;
            g_devices[i] = NULL;
            if (g_device_count > 0U) g_device_count--;
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_INVALID_ARGUMENT;
}

block_device_t *block_device_get(io_device_id_t device_id) {
    for (uint32_t i = 0; i < MAX_REG_DEVICES; ++i) {
        if (g_devices[i] && g_devices[i]->id == device_id) {
            return g_devices[i];
        }
    }
    return NULL;
}

io_status_t block_device_enumerate(io_device_id_t *out_ids, uint32_t capacity, uint32_t *out_count) {
    if (!out_ids || !out_count) return IO_STATUS_INVALID_ARGUMENT;

    uint32_t count = 0U;
    for (uint32_t i = 0; i < MAX_REG_DEVICES; ++i) {
        if (g_devices[i]) {
            if (count < capacity) {
                out_ids[count] = g_devices[i]->id;
            }
            count++;
        }
    }

    *out_count = count;
    return (count <= capacity) ? IO_STATUS_OK : IO_STATUS_QUEUE_FULL;
}

io_status_t block_device_find_by_role(io_device_role_t role, io_device_id_t *out_device_id) {
    if (!out_device_id) return IO_STATUS_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < MAX_REG_DEVICES; ++i) {
        if (g_devices[i] && g_devices[i]->role == role) {
            *out_device_id = g_devices[i]->id;
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_INVALID_ARGUMENT;
}

io_status_t block_channel_open(io_device_id_t device_id, const io_channel_config_t *config, void **out_channel) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;
    if (!dev->ops || !dev->ops->open_channel) return IO_STATUS_NOT_SUPPORTED;

    return dev->ops->open_channel(dev->driver_context, config, out_channel);
}

io_status_t block_channel_close(io_device_id_t device_id, void *channel) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;
    if (!dev->ops || !dev->ops->close_channel) return IO_STATUS_NOT_SUPPORTED;

    return dev->ops->close_channel(channel);
}

io_status_t block_submit(io_device_id_t device_id, void *channel, const io_request_t *request) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;
    if (!dev->ops || !dev->ops->submit) return IO_STATUS_NOT_SUPPORTED;

    return dev->ops->submit(channel, request);
}

io_status_t block_submit_batch(io_device_id_t device_id, void *channel, const io_request_t *requests, uint32_t request_count, uint32_t *accepted_count) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;
    if (!dev->ops) return IO_STATUS_NOT_SUPPORTED;

    if (dev->ops->submit_batch) {
        return dev->ops->submit_batch(channel, requests, request_count, accepted_count);
    }

    // Fallback: submit individually
    if (!dev->ops->submit) return IO_STATUS_NOT_SUPPORTED;
    uint32_t accepted = 0U;
    for (uint32_t i = 0; i < request_count; ++i) {
        io_status_t status = dev->ops->submit(channel, &requests[i]);
        if (status == IO_STATUS_OK) {
            accepted++;
        } else {
            break;
        }
    }
    if (accepted_count) *accepted_count = accepted;
    return (accepted > 0U) ? IO_STATUS_OK : IO_STATUS_QUEUE_FULL;
}

io_status_t block_poll_completions(io_device_id_t device_id, void *channel, io_completion_t *completions, uint32_t capacity, uint32_t *completion_count) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;
    if (!dev->ops || !dev->ops->poll_completions) return IO_STATUS_NOT_SUPPORTED;

    return dev->ops->poll_completions(channel, completions, capacity, completion_count);
}

io_status_t block_cancel(io_device_id_t device_id, void *channel, io_request_id_t request_id) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev) return IO_STATUS_INVALID_ARGUMENT;
    if (!dev->ops || !dev->ops->cancel) return IO_STATUS_NOT_SUPPORTED;

    return dev->ops->cancel(channel, request_id);
}

io_status_t block_device_resolve_policy(io_device_id_t device_id, io_device_policy_t *out_policy) {
    block_device_t *dev = block_device_get(device_id);
    if (!dev || !out_policy) return IO_STATUS_INVALID_ARGUMENT;

    out_policy->device_id = device_id;
    out_policy->max_transfer_bytes = dev->caps.max_transfer_bytes;

    if (dev->caps.media_class == IO_MEDIA_RAM) {
        out_policy->queue_policy = IO_QUEUE_POLICY_SINGLE_OWNER;
        out_policy->completion_policy = IO_COMPLETION_POLICY_POLL;
        out_policy->cache_policy = IO_CACHE_POLICY_NONE;
        out_policy->durability_policy = IO_DURABILITY_POLICY_LAZY;
        out_policy->power_policy = IO_POWER_POLICY_PERFORMANCE;
        out_policy->queue_count = 1U;
        out_policy->queue_depth = dev->caps.max_queue_depth;
        out_policy->cache_budget_bytes = 0ULL;
        out_policy->allow_direct_io = true;
        out_policy->allow_request_merge = false;
        out_policy->allow_readahead = false;
    } else if (dev->caps.media_class == IO_MEDIA_SSD_NVME) {
        out_policy->queue_policy = IO_QUEUE_POLICY_PER_CORE;
        out_policy->completion_policy = IO_COMPLETION_POLICY_HYBRID;
        out_policy->cache_policy = IO_CACHE_POLICY_NUMA;
        out_policy->durability_policy = IO_DURABILITY_POLICY_STRICT_FUA;
        out_policy->power_policy = IO_POWER_POLICY_PERFORMANCE;
        out_policy->queue_count = dev->caps.max_hw_queues;
        out_policy->queue_depth = dev->caps.max_queue_depth;
        out_policy->cache_budget_bytes = 16ULL * 1024ULL * 1024ULL;
        out_policy->allow_direct_io = true;
        out_policy->allow_request_merge = true;
        out_policy->allow_readahead = true;
    } else if (dev->caps.media_class == IO_MEDIA_SD || dev->caps.media_class == IO_MEDIA_EMMC) {
        out_policy->queue_policy = IO_QUEUE_POLICY_SINGLE_OWNER;
        out_policy->completion_policy = IO_COMPLETION_POLICY_INTERRUPT;
        out_policy->cache_policy = IO_CACHE_POLICY_WRITEBACK;
        out_policy->durability_policy = IO_DURABILITY_POLICY_WRITE_THROUGH;
        out_policy->power_policy = IO_POWER_POLICY_BALANCED;
        out_policy->queue_count = 1U;
        out_policy->queue_depth = dev->caps.max_queue_depth;
        out_policy->cache_budget_bytes = 256ULL * 1024ULL;
        out_policy->allow_direct_io = false;
        out_policy->allow_request_merge = true;
        out_policy->allow_readahead = true;
    } else {
        out_policy->queue_policy = IO_QUEUE_POLICY_SHARED_MP;
        out_policy->completion_policy = IO_COMPLETION_POLICY_INTERRUPT;
        out_policy->cache_policy = IO_CACHE_POLICY_FIXED;
        out_policy->durability_policy = IO_DURABILITY_POLICY_LAZY;
        out_policy->power_policy = IO_POWER_POLICY_BALANCED;
        out_policy->queue_count = 1U;
        out_policy->queue_depth = dev->caps.max_queue_depth;
        out_policy->cache_budget_bytes = 1024ULL * 1024ULL;
        out_policy->allow_direct_io = true;
        out_policy->allow_request_merge = true;
        out_policy->allow_readahead = true;
    }

    return IO_STATUS_OK;
}

/* Compatibility legacy wrappers */

int block_queue_request(uint32_t device_id, block_request_t* req) {
    if (!req) return -1;

    block_device_t *dev = block_device_get(device_id);
    if (!dev) {
        req->status = -1;
        return -1;
    }

    // Translate request type
    io_opcode_t opcode = IO_OP_READ;
    if (req->type == BLOCK_REQ_WRITE) {
        opcode = IO_OP_WRITE;
    } else if (req->type == BLOCK_REQ_FLUSH) {
        opcode = IO_OP_FLUSH;
    }

    io_channel_config_t config = {
        .queue_id = 0,
        .queue_depth = 16,
        .enable_polling = false
    };

    void *channel = NULL;
    if (block_channel_open(device_id, &config, &channel) != IO_STATUS_OK) {
        req->status = -1;
        return -1;
    }

    io_request_t io_req = {
        .id = 1000ULL + (uintptr_t)req, // Unique mock ID
        .device_id = device_id,
        .opcode = opcode,
        .lba = req->lba,
        .block_count = req->num_blocks,
        .buffer_cap = 0,
        .segments = NULL,
        .segment_count = 0,
        .priority = 0,
        .deadline_ns = 0,
        .flags = 0,
        .completion_cookie = req
    };

    // If flush, map request parameters to flush behavior
    if (opcode == IO_OP_FLUSH) {
        io_req.block_count = 0;
    }

    if (block_submit(device_id, channel, &io_req) != IO_STATUS_OK) {
        block_channel_close(device_id, channel);
        req->status = -1;
        return -1;
    }

    // Synchronously poll completions until our request finishes
    bool completed = false;
    extern void memblk_tick(void) __attribute__((weak));

    for (uint32_t retries = 0; retries < 100000U; ++retries) {
        if (memblk_tick) {
            memblk_tick();
        }

        io_completion_t completions[4];
        uint32_t comp_count = 0U;
        if (block_poll_completions(device_id, channel, completions, 4, &comp_count) == IO_STATUS_OK) {
            for (uint32_t c = 0; c < comp_count; ++c) {
                if (completions[c].id == io_req.id) {
                    req->status = (completions[c].status == IO_STATUS_OK) ? 0 : -1;
                    completed = true;
                    break;
                }
            }
        }
        if (completed) break;
    }

    block_channel_close(device_id, channel);
    return completed ? req->status : -1;
}

int block_get_info(uint32_t device_id, block_device_info_t* info) {
    if (!info) return -1;

    block_device_t *dev = block_device_get(device_id);
    if (!dev) return -1;

    info->device_id = device_id;
    info->block_size = dev->caps.logical_block_size;
    info->total_blocks = dev->caps.capacity_bytes / dev->caps.logical_block_size;
    return 0;
}

#ifdef BHARAT_IO_DRIVER_MEMBLK
__attribute__((weak)) void memblk_init(void);
#endif

void block_stacks_init(void) {
#ifdef BHARAT_IO_DRIVER_MEMBLK
    if (memblk_init) {
        memblk_init();
    }
#endif
}
