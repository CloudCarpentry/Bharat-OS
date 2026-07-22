#include "memblk.h"
#include <bharat/stacks/storage/block.h>
#include <bharat/stacks/storage/block_driver.h>
#include <lib/base/string.h>

#define MEMBLK_SECTOR_SIZE 512U
#define MEMBLK_TOTAL_SECTORS 512U // 256 KB size
#define MEMBLK_MAX_CHANNELS 4U
#define MEMBLK_QUEUE_DEPTH 16U

typedef struct {
    io_request_t req;
    bool active;
    uint64_t submit_time;
} memblk_pending_req_t;

typedef struct {
    bool in_use;
    memblk_pending_req_t queue[MEMBLK_QUEUE_DEPTH];
    io_completion_t completions[MEMBLK_QUEUE_DEPTH];
    uint32_t comp_head;
    uint32_t comp_tail;
    uint32_t comp_count;
} memblk_channel_t;

typedef struct {
    io_device_id_t id;
    uint8_t storage[MEMBLK_TOTAL_SECTORS * MEMBLK_SECTOR_SIZE];
    bool inject_error;
    io_status_t fault_status;
    uint32_t inject_delay;
    memblk_channel_t channels[MEMBLK_MAX_CHANNELS];
    block_device_t dev;
} memblk_instance_t;

static memblk_instance_t g_instances[2];
static bool g_registered = false;

// Persistent backing storage for simulation
static uint8_t g_persistent_backing[2][MEMBLK_TOTAL_SECTORS * MEMBLK_SECTOR_SIZE];
static bool g_backing_enabled[2] = {false, false};

static memblk_instance_t *find_instance_by_channel(void *channel) {
    for (uint32_t i = 0; i < 2; ++i) {
        for (uint32_t c = 0; c < MEMBLK_MAX_CHANNELS; ++c) {
            if (&g_instances[i].channels[c] == channel) {
                return &g_instances[i];
            }
        }
    }
    return NULL;
}

static memblk_instance_t *find_instance_by_device_id(io_device_id_t id) {
    for (uint32_t i = 0; i < 2; ++i) {
        if (g_instances[i].id == id) {
            return &g_instances[i];
        }
    }
    return NULL;
}

// Driver operations
static io_status_t memblk_probe(void *driver_ctx) {
    (void)driver_ctx;
    return IO_STATUS_OK;
}

static io_status_t memblk_start(void *driver_ctx) {
    (void)driver_ctx;
    return IO_STATUS_OK;
}

static io_status_t memblk_stop(void *driver_ctx) {
    (void)driver_ctx;
    return IO_STATUS_OK;
}

static io_status_t memblk_get_caps(void *driver_ctx, io_device_caps_t *out_caps) {
    memblk_instance_t *inst = (memblk_instance_t *)driver_ctx;
    uint32_t idx = 0;
    if (inst && inst->id == 43U) {
        idx = 1;
    }

    if (!out_caps) return IO_STATUS_INVALID_ARGUMENT;

    out_caps->media_class = IO_MEDIA_RAM;
    out_caps->transport = IO_TRANSPORT_RAM;
    out_caps->logical_block_size = MEMBLK_SECTOR_SIZE;
    out_caps->physical_block_size = MEMBLK_SECTOR_SIZE;
    out_caps->optimal_io_size = MEMBLK_SECTOR_SIZE;
    out_caps->erase_block_size = MEMBLK_SECTOR_SIZE * 8U;
    out_caps->capacity_bytes = MEMBLK_TOTAL_SECTORS * MEMBLK_SECTOR_SIZE;
    out_caps->max_transfer_bytes = MEMBLK_SECTOR_SIZE * 32U;
    out_caps->max_segments = 8U;
    out_caps->max_hw_queues = 1U;
    out_caps->max_queue_depth = MEMBLK_QUEUE_DEPTH;
    out_caps->interrupt_vectors = 0U;
    out_caps->rotational = false;
    out_caps->removable = false;
    out_caps->read_only = false;
    out_caps->dma_supported = true;
    out_caps->dma_coherent = true;
    out_caps->scatter_gather_supported = true;
    out_caps->flush_supported = true;
    out_caps->fua_supported = true;
    out_caps->discard_supported = true;
    out_caps->write_zeroes_supported = true;
    out_caps->volatile_write_cache = g_backing_enabled[idx];
    out_caps->power_loss_protection = !g_backing_enabled[idx];
    out_caps->polling_supported = true;
    out_caps->hybrid_completion_supported = false;

    return IO_STATUS_OK;
}

static io_status_t memblk_open_channel(void *driver_ctx, const io_channel_config_t *config, void **out_channel) {
    if (!out_channel) return IO_STATUS_INVALID_ARGUMENT;
    memblk_instance_t *inst = (memblk_instance_t *)driver_ctx;
    if (!inst) {
        inst = &g_instances[0];
    }

    for (uint32_t i = 0; i < MEMBLK_MAX_CHANNELS; ++i) {
        if (!inst->channels[i].in_use) {
            inst->channels[i].in_use = true;
            inst->channels[i].comp_head = 0U;
            inst->channels[i].comp_tail = 0U;
            inst->channels[i].comp_count = 0U;
            for (uint32_t q = 0; q < MEMBLK_QUEUE_DEPTH; ++q) {
                inst->channels[i].queue[q].active = false;
            }
            *out_channel = &inst->channels[i];
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_QUEUE_FULL;
}

static io_status_t memblk_close_channel(void *channel) {
    if (!channel) return IO_STATUS_INVALID_ARGUMENT;
    memblk_channel_t *chan = (memblk_channel_t *)channel;
    chan->in_use = false;
    return IO_STATUS_OK;
}

static io_status_t memblk_submit(void *channel, const io_request_t *request) {
    if (!channel || !request) return IO_STATUS_INVALID_ARGUMENT;
    memblk_channel_t *chan = (memblk_channel_t *)channel;

    // Check backpressure / queue full
    uint32_t active_count = 0;
    for (uint32_t i = 0; i < MEMBLK_QUEUE_DEPTH; ++i) {
        if (chan->queue[i].active) {
            active_count++;
        }
    }
    if (active_count >= MEMBLK_QUEUE_DEPTH) {
        return IO_STATUS_QUEUE_FULL;
    }

    // Insert into request queue
    for (uint32_t i = 0; i < MEMBLK_QUEUE_DEPTH; ++i) {
        if (!chan->queue[i].active) {
            chan->queue[i].req = *request;
            chan->queue[i].active = true;
            chan->queue[i].submit_time = 0;
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_QUEUE_FULL;
}

static io_status_t memblk_poll_completions(void *channel, io_completion_t *completions, uint32_t capacity, uint32_t *completion_count) {
    if (!channel || !completions || !completion_count) return IO_STATUS_INVALID_ARGUMENT;
    memblk_channel_t *chan = (memblk_channel_t *)channel;

    uint32_t count = 0U;
    while (count < capacity && chan->comp_count > 0U) {
        completions[count] = chan->completions[chan->comp_head];
        chan->comp_head = (chan->comp_head + 1U) % MEMBLK_QUEUE_DEPTH;
        chan->comp_count--;
        count++;
    }

    *completion_count = count;
    return IO_STATUS_OK;
}

static io_status_t memblk_cancel(void *channel, io_request_id_t request_id) {
    if (!channel) return IO_STATUS_INVALID_ARGUMENT;
    memblk_channel_t *chan = (memblk_channel_t *)channel;

    // Search active requests
    for (uint32_t i = 0; i < MEMBLK_QUEUE_DEPTH; ++i) {
        if (chan->queue[i].active && chan->queue[i].req.id == request_id) {
            chan->queue[i].active = false; // Deactivate

            // Push CANCELLED completion
            if (chan->comp_count < MEMBLK_QUEUE_DEPTH) {
                io_completion_t comp = {
                    .id = request_id,
                    .device_id = chan->queue[i].req.device_id,
                    .status = IO_STATUS_CANCELLED,
                    .transferred_blocks = 0,
                    .completion_time_ns = 0,
                    .completion_cookie = chan->queue[i].req.completion_cookie
                };
                chan->completions[chan->comp_tail] = comp;
                chan->comp_tail = (chan->comp_tail + 1U) % MEMBLK_QUEUE_DEPTH;
                chan->comp_count++;
            }
            return IO_STATUS_OK;
        }
    }

    return IO_STATUS_INVALID_ARGUMENT; // Not found
}

static io_status_t memblk_flush(void *channel, io_request_id_t request_id) {
    io_request_t req = {
        .id = request_id,
        .opcode = IO_OP_FLUSH,
        .block_count = 0,
        .lba = 0
    };
    return memblk_submit(channel, &req);
}

static io_status_t memblk_discard(void *channel, uint64_t lba, uint32_t block_count, io_request_id_t request_id) {
    io_request_t req = {
        .id = request_id,
        .opcode = IO_OP_DISCARD,
        .block_count = block_count,
        .lba = lba
    };
    return memblk_submit(channel, &req);
}

static io_status_t memblk_reset(void *driver_ctx) {
    memblk_instance_t *inst = (memblk_instance_t *)driver_ctx;
    if (!inst) return IO_STATUS_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < MEMBLK_MAX_CHANNELS; ++i) {
        inst->channels[i].comp_head = 0U;
        inst->channels[i].comp_tail = 0U;
        inst->channels[i].comp_count = 0U;
        for (uint32_t q = 0; q < MEMBLK_QUEUE_DEPTH; ++q) {
            inst->channels[i].queue[q].active = false;
        }
    }
    return IO_STATUS_OK;
}

static const block_driver_ops_t g_memblk_ops = {
    .probe = memblk_probe,
    .start = memblk_start,
    .stop = memblk_stop,
    .get_caps = memblk_get_caps,
    .open_channel = memblk_open_channel,
    .close_channel = memblk_close_channel,
    .submit = memblk_submit,
    .submit_batch = NULL,
    .poll_completions = memblk_poll_completions,
    .cancel = memblk_cancel,
    .flush = memblk_flush,
    .discard = memblk_discard,
    .reset = memblk_reset
};

void memblk_init(void) {
    if (g_registered) return;

    memset(g_instances, 0, sizeof(g_instances));

    // memblk0
    g_instances[0].id = 42U;
    memblk_get_caps(&g_instances[0], &g_instances[0].dev.caps);
    g_instances[0].dev.id = g_instances[0].id;
    g_instances[0].dev.name = "memblk0";
    g_instances[0].dev.ops = &g_memblk_ops;
    g_instances[0].dev.driver_context = &g_instances[0];
    g_instances[0].dev.state = IO_DEV_STATE_READY;
    g_instances[0].dev.role = IO_DEVICE_ROLE_SYSTEM;

    if (g_backing_enabled[0]) {
        memcpy(g_instances[0].storage, g_persistent_backing[0], sizeof(g_instances[0].storage));
    }

    // memblk1
    g_instances[1].id = 43U;
    memblk_get_caps(&g_instances[1], &g_instances[1].dev.caps);
    g_instances[1].dev.id = g_instances[1].id;
    g_instances[1].dev.name = "memblk1";
    g_instances[1].dev.ops = &g_memblk_ops;
    g_instances[1].dev.driver_context = &g_instances[1];
    g_instances[1].dev.state = IO_DEV_STATE_READY;
    g_instances[1].dev.role = IO_DEVICE_ROLE_DATA;

    if (g_backing_enabled[1]) {
        memcpy(g_instances[1].storage, g_persistent_backing[1], sizeof(g_instances[1].storage));
    }

    block_driver_register(&g_memblk_ops);
    block_device_register(&g_instances[0].dev);
    block_device_register(&g_instances[1].dev);
    g_registered = true;
}

void memblk_deinit(void) {
    if (!g_registered) return;
    block_device_unregister(g_instances[0].id);
    block_device_unregister(g_instances[1].id);
    block_driver_unregister(&g_memblk_ops);
    g_registered = false;
}

void memblk_inject_error(bool enable) {
    g_instances[0].inject_error = enable;
    g_instances[1].inject_error = enable;
}

void memblk_inject_delay(uint32_t delay_loops) {
    g_instances[0].inject_delay = delay_loops;
    g_instances[1].inject_delay = delay_loops;
}

void memblk_attach_backing(uint32_t device_id, bool enable) {
    uint32_t idx = (device_id == 43U) ? 1 : 0;
    g_backing_enabled[idx] = enable;
    memblk_instance_t *inst = find_instance_by_device_id(device_id);
    if (inst) {
        inst->dev.caps.volatile_write_cache = enable;
        inst->dev.caps.power_loss_protection = !enable;
        if (enable) {
            memcpy(g_persistent_backing[idx], inst->storage, sizeof(inst->storage));
        }
    }
}

void memblk_power_cycle(uint32_t device_id) {
    uint32_t idx = (device_id == 43U) ? 1 : 0;
    memblk_instance_t *inst = find_instance_by_device_id(device_id);
    if (inst) {
        // Discard channels, queues, and completions
        for (uint32_t c = 0; c < MEMBLK_MAX_CHANNELS; ++c) {
            inst->channels[c].in_use = false;
            inst->channels[c].comp_head = 0U;
            inst->channels[c].comp_tail = 0U;
            inst->channels[c].comp_count = 0U;
            for (uint32_t q = 0; q < MEMBLK_QUEUE_DEPTH; ++q) {
                inst->channels[q].queue[q].active = false;
            }
        }
        // Restore storage from persistent backing to simulate loss of volatile controller cache!
        if (g_backing_enabled[idx]) {
            memcpy(inst->storage, g_persistent_backing[idx], sizeof(inst->storage));
        }
    }
}

void memblk_reopen(uint32_t device_id) {
    memblk_instance_t *inst = find_instance_by_device_id(device_id);
    if (inst) {
        memblk_reset(inst);
    }
}

// Copy helper to support scatter-gather segments as well as raw buffer fallbacks
static void perform_io_copy(memblk_instance_t *inst, const io_request_t *req, bool is_write) {
    uint64_t start_offset = req->lba * MEMBLK_SECTOR_SIZE;
    uint32_t total_bytes = req->block_count * MEMBLK_SECTOR_SIZE;

    if (start_offset + total_bytes > sizeof(inst->storage)) {
        return; // Out of bounds safety
    }

    if (req->segment_count > 0 && req->segments) {
        // Multi-segment SG copy
        uint32_t storage_cursor = (uint32_t)start_offset;
        for (uint16_t s = 0; s < req->segment_count; ++s) {
            void *segment_ptr = (void *)(uintptr_t)req->segments[s].phys_addr;
            uint32_t len = req->segments[s].length;
            if (storage_cursor + len > sizeof(inst->storage)) {
                len = sizeof(inst->storage) - storage_cursor;
            }
            if (is_write) {
                memcpy(&inst->storage[storage_cursor], segment_ptr, len);
            } else {
                memcpy(segment_ptr, &inst->storage[storage_cursor], len);
            }
            storage_cursor += len;
        }
    } else {
        // Direct buffer fallback
        void *direct_ptr = (void *)(uintptr_t)req->buffer_cap;
        if (!direct_ptr) {
            block_request_t *compat_req = (block_request_t *)req->completion_cookie;
            if (compat_req) {
                direct_ptr = compat_req->buffer;
            }
        }

        if (direct_ptr) {
            if (is_write) {
                memcpy(&inst->storage[start_offset], direct_ptr, total_bytes);
            } else {
                memcpy(direct_ptr, &inst->storage[start_offset], total_bytes);
            }
        }
    }

    // Handle persistent commit if non-volatile or FUA write
    uint32_t idx = (inst->id == 43U) ? 1 : 0;
    if (is_write && g_backing_enabled[idx]) {
        if ((req->flags & IO_REQ_FUA) || !inst->dev.caps.volatile_write_cache) {
            memcpy(g_persistent_backing[idx] + start_offset, &inst->storage[start_offset], total_bytes);
        }
    }
}

static void tick_instance(memblk_instance_t *inst) {
    for (uint32_t c = 0; c < MEMBLK_MAX_CHANNELS; ++c) {
        if (!inst->channels[c].in_use) continue;
        memblk_channel_t *chan = &inst->channels[c];

        for (uint32_t i = 0; i < MEMBLK_QUEUE_DEPTH; ++i) {
            if (chan->queue[i].active) {
                io_request_t *req = &chan->queue[i].req;

                if (inst->inject_delay > 0U) {
                    volatile uint32_t d = inst->inject_delay;
                    while (d--) {}
                }

                io_status_t final_status = IO_STATUS_OK;
                uint32_t transferred = req->block_count;

                if (inst->inject_error || inst->fault_status != IO_STATUS_OK) {
                    final_status = (inst->fault_status != IO_STATUS_OK) ? inst->fault_status : IO_STATUS_IO_ERROR;
                    transferred = 0U;
                } else {
                    if (req->opcode == IO_OP_READ) {
                        perform_io_copy(inst, req, false);
                    } else if (req->opcode == IO_OP_WRITE) {
                        perform_io_copy(inst, req, true);
                    } else if (req->opcode == IO_OP_FLUSH) {
                        uint32_t idx = (inst->id == 43U) ? 1 : 0;
                        if (g_backing_enabled[idx]) {
                            memcpy(g_persistent_backing[idx], inst->storage, sizeof(inst->storage));
                        }
                    } else if (req->opcode == IO_OP_DISCARD) {
                        uint64_t start_offset = req->lba * MEMBLK_SECTOR_SIZE;
                        uint32_t total_bytes = req->block_count * MEMBLK_SECTOR_SIZE;
                        if (start_offset + total_bytes <= sizeof(inst->storage)) {
                            memset(&inst->storage[start_offset], 0, total_bytes);
                            uint32_t idx = (inst->id == 43U) ? 1 : 0;
                            if (g_backing_enabled[idx]) {
                                memset(g_persistent_backing[idx] + start_offset, 0, total_bytes);
                            }
                        }
                    }
                }

                // Deactivate the request
                chan->queue[i].active = false;

                // Push completion
                if (chan->comp_count < MEMBLK_QUEUE_DEPTH) {
                    io_completion_t comp = {
                        .id = req->id,
                        .device_id = req->device_id,
                        .status = final_status,
                        .transferred_blocks = transferred,
                        .completion_time_ns = 100ULL,
                        .completion_cookie = req->completion_cookie
                    };
                    chan->completions[chan->comp_tail] = comp;
                    chan->comp_tail = (chan->comp_tail + 1U) % MEMBLK_QUEUE_DEPTH;
                    chan->comp_count++;
                }
            }
        }
    }
}

void memblk_tick(void) {
    tick_instance(&g_instances[0]);
    tick_instance(&g_instances[1]);
}

void memblk_test_advance(io_device_id_t device_id, uint32_t steps) {
    memblk_instance_t *inst = find_instance_by_device_id(device_id);
    if (!inst) return;

    while (steps--) {
        tick_instance(inst);
    }
}

void memblk_test_complete_next(io_device_id_t device_id) {
    memblk_instance_t *inst = find_instance_by_device_id(device_id);
    if (!inst) return;

    // Process exactly one active request
    for (uint32_t c = 0; c < MEMBLK_MAX_CHANNELS; ++c) {
        if (!inst->channels[c].in_use) continue;
        memblk_channel_t *chan = &inst->channels[c];

        for (uint32_t i = 0; i < MEMBLK_QUEUE_DEPTH; ++i) {
            if (chan->queue[i].active) {
                io_request_t *req = &chan->queue[i].req;
                io_status_t final_status = (inst->fault_status != IO_STATUS_OK) ? inst->fault_status : IO_STATUS_OK;
                uint32_t transferred = (final_status == IO_STATUS_OK) ? req->block_count : 0U;

                if (final_status == IO_STATUS_OK) {
                    if (req->opcode == IO_OP_READ) {
                        perform_io_copy(inst, req, false);
                    } else if (req->opcode == IO_OP_WRITE) {
                        perform_io_copy(inst, req, true);
                    }
                }

                chan->queue[i].active = false;

                if (chan->comp_count < MEMBLK_QUEUE_DEPTH) {
                    io_completion_t comp = {
                        .id = req->id,
                        .device_id = req->device_id,
                        .status = final_status,
                        .transferred_blocks = transferred,
                        .completion_time_ns = 100ULL,
                        .completion_cookie = req->completion_cookie
                    };
                    chan->completions[chan->comp_tail] = comp;
                    chan->comp_tail = (chan->comp_tail + 1U) % MEMBLK_QUEUE_DEPTH;
                    chan->comp_count++;
                }
                return; // process exactly one
            }
        }
    }
}

void memblk_test_set_fault(io_device_id_t device_id, io_status_t fault_status) {
    memblk_instance_t *inst = find_instance_by_device_id(device_id);
    if (inst) {
        inst->fault_status = fault_status;
        inst->inject_error = (fault_status != IO_STATUS_OK);
    }
}
