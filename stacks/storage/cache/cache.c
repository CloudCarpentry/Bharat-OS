#include "bharat/stacks/storage/cache/cache.h"

#include <stddef.h>

#define CACHE_DEFAULT_CAPACITY 256U
#define CACHE_MAX_CAPACITY 4096U

typedef struct {
    cache_block_t block;
    uint8_t in_use;
    uint8_t pin_count;
} cache_entry_t;

static cache_entry_t g_cache[CACHE_MAX_CAPACITY];
static uint32_t g_cache_capacity = CACHE_DEFAULT_CAPACITY;
static uint32_t g_clock_hand = 0U;

static cache_entry_t* cache_find(uint32_t device_id, uint64_t lba) {
    for (uint32_t i = 0; i < g_cache_capacity; ++i) {
        if (g_cache[i].in_use && g_cache[i].block.device_id == device_id && g_cache[i].block.lba == lba) {
            return &g_cache[i];
        }
    }
    return NULL;
}

static cache_entry_t* cache_pick_victim(void) {
    for (uint32_t probe = 0; probe < g_cache_capacity; ++probe) {
        uint32_t idx = (g_clock_hand + probe) % g_cache_capacity;
        if (!g_cache[idx].in_use || g_cache[idx].pin_count == 0U) {
            g_clock_hand = (idx + 1U) % g_cache_capacity;
            return &g_cache[idx];
        }
    }
    return NULL;
}

int cache_init(void) {
    storage_profile_config_t cfg;
    if (storage_profile_resolve(STORAGE_APP_PROFILE_EDGE,
                                8ULL * 1024ULL * 1024ULL * 1024ULL,
                                STORAGE_HW_ARCH_UNKNOWN,
                                &cfg) != 0) {
        return -1;
    }
    return cache_init_with_profile(&cfg);
}

int cache_init_with_profile(const storage_profile_config_t* cfg) {
    if (!cfg) {
        return -1;
    }

    uint32_t requested = cfg->cache_block_budget;
    if (requested < 32U) {
        requested = 32U;
    }
    if (requested > CACHE_MAX_CAPACITY) {
        requested = CACHE_MAX_CAPACITY;
    }
    g_cache_capacity = requested;
    g_clock_hand = 0U;

    for (uint32_t i = 0; i < CACHE_MAX_CAPACITY; ++i) {
        g_cache[i].in_use = 0U;
        g_cache[i].pin_count = 0U;
        g_cache[i].block.device_id = 0U;
        g_cache[i].block.lba = 0U;
        g_cache[i].block.flags = 0U;
        g_cache[i].block.buffer = NULL;
    }

    return 0;
}

int cache_get_block(uint32_t device_id, uint64_t lba, cache_block_t** out_block) {
    if (!out_block) {
        return -1;
    }

    cache_entry_t* hit = cache_find(device_id, lba);
    if (hit) {
        if (hit->pin_count < 0xFFU) {
            hit->pin_count++;
        }
        *out_block = &hit->block;
        return 0;
    }

    cache_entry_t* slot = cache_pick_victim();
    if (!slot) {
        return -1;
    }

    if (slot->in_use && (slot->block.flags & CACHE_BLOCK_DIRTY) != 0U && slot->block.buffer) {
        block_request_t req = {
            .type = BLOCK_REQ_WRITE,
            .lba = slot->block.lba,
            .num_blocks = 1,
            .buffer = slot->block.buffer,
            .status = -1,
        };
        (void)block_queue_request(slot->block.device_id, &req);
    }

    slot->in_use = 1U;
    slot->pin_count = 1U;
    slot->block.device_id = device_id;
    slot->block.lba = lba;
    slot->block.flags = CACHE_BLOCK_VALID;
    slot->block.buffer = NULL;

    *out_block = &slot->block;
    return 0;
}

int cache_put_block(cache_block_t* block) {
    if (!block) {
        return -1;
    }

    for (uint32_t i = 0; i < g_cache_capacity; ++i) {
        if (g_cache[i].in_use && (&g_cache[i].block == block)) {
            if (g_cache[i].pin_count > 0U) {
                g_cache[i].pin_count--;
            }
            return 0;
        }
    }

    return -1;
}

int cache_sync(uint32_t device_id) {
    for (uint32_t i = 0; i < g_cache_capacity; ++i) {
        if (!g_cache[i].in_use || g_cache[i].block.device_id != device_id) {
            continue;
        }

        if ((g_cache[i].block.flags & CACHE_BLOCK_DIRTY) != 0U && g_cache[i].block.buffer) {
            block_request_t req = {
                .type = BLOCK_REQ_WRITE,
                .lba = g_cache[i].block.lba,
                .num_blocks = 1,
                .buffer = g_cache[i].block.buffer,
                .status = -1,
            };
            (void)block_queue_request(device_id, &req);
            g_cache[i].block.flags &= ~CACHE_BLOCK_DIRTY;
        }
    }

    block_request_t flush_req = {
        .type = BLOCK_REQ_FLUSH,
        .lba = 0U,
        .num_blocks = 0U,
        .buffer = NULL,
        .status = -1,
    };
    return block_queue_request(device_id, &flush_req);
}
