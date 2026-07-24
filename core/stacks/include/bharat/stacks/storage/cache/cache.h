#pragma once

#include "bharat/stacks/storage/block.h"
#include "bharat/stacks/storage/profile.h"

// Buffer/Cache Abstraction
#define CACHE_BLOCK_VALID 0x1U
#define CACHE_BLOCK_DIRTY 0x2U

typedef struct {
    uint32_t device_id;
    uint64_t lba;
    uint32_t flags; // DIRTY, VALID, etc.
    void* buffer;   // DMA-safe page buffer
} cache_block_t;

int cache_init(void);
int cache_init_with_profile(const storage_profile_config_t* cfg);
int cache_get_block(uint32_t device_id, uint64_t lba, cache_block_t** out_block);
int cache_put_block(cache_block_t* block);
int cache_sync(uint32_t device_id);
