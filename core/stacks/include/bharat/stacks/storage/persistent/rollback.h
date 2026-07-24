#ifndef BHARAT_STACKS_STORAGE_PERSISTENT_ROLLBACK_H
#define BHARAT_STACKS_STORAGE_PERSISTENT_ROLLBACK_H

#include "bharat/stacks/storage/block.h"

typedef struct {
    io_status_t (*read_epoch)(void *ctx, uint64_t *epoch);
    io_status_t (*advance_epoch)(void *ctx, uint64_t expected, uint64_t next);
    io_status_t (*bind_digest)(
        void *ctx,
        uint64_t epoch,
        const uint8_t *digest,
        size_t digest_len);
} rollback_anchor_ops_t;

typedef enum {
    ROLLBACK_PROTECTION_NONE,
    ROLLBACK_PROTECTION_INTEGRITY_ONLY,
    ROLLBACK_PROTECTION_DETECTION,
    ROLLBACK_PROTECTION_PREVENTION
} rollback_protection_level_t;

// Test variables to control rollback level in host tests
extern rollback_protection_level_t g_test_rollback_level;
extern uint64_t g_test_rollback_epoch;

#endif /* BHARAT_STACKS_STORAGE_PERSISTENT_ROLLBACK_H */
