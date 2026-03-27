#include "bharat/stacks/storage/cache/cache.h"
#include <stddef.h>

int cache_init(void) {
    // Stub
    return 0;
}

int cache_get_block(uint32_t device_id, uint64_t lba, cache_block_t** out_block) {
    if (!out_block) return -1;
    // Stub: Simulate hit
    *out_block = NULL;
    return -1; // Not fully implemented
}

int cache_put_block(cache_block_t* block) {
    if (!block) return -1;
    // Stub
    return 0;
}

int cache_sync(uint32_t device_id) {
    // Stub
    return 0;
}
