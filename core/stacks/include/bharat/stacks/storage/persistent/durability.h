#ifndef BHARAT_STACKS_STORAGE_PERSISTENT_DURABILITY_H
#define BHARAT_STACKS_STORAGE_PERSISTENT_DURABILITY_H

#include "bharat/stacks/storage/block.h"

typedef enum {
    IO_DURABILITY_UNSAFE,
    IO_DURABILITY_PROCESS_VISIBLE,
    IO_DURABILITY_DEVICE_FLUSHED,
    IO_DURABILITY_POWER_LOSS_PROTECTED,
    IO_DURABILITY_ROLLBACK_PROTECTED
} io_durability_level_t;

typedef enum {
    DURABILITY_SUPPORTED,
    DURABILITY_SUPPORTED_WITH_FLUSH,
    DURABILITY_SUPPORTED_WITH_FUA,
    DURABILITY_UNSAFE_BUT_ALLOWED,
    DURABILITY_NOT_SUPPORTED
} storage_durability_status_t;

typedef struct {
    storage_durability_status_t status;
} storage_durability_result_t;

io_status_t storage_durability_evaluate(
    const io_device_caps_t *caps,
    io_durability_level_t requested,
    storage_durability_result_t *out);

#endif /* BHARAT_STACKS_STORAGE_PERSISTENT_DURABILITY_H */
