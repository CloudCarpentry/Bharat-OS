#include "bh_personality_registry.h"
#include <stddef.h>

static const void* personality_table[BH_PERSONALITY_MAX + 1];

void bh_personality_registry_register(bh_personality_kind_t kind, const void *ops) {
    if (kind <= BH_PERSONALITY_MAX) {
        personality_table[kind] = ops;
    }
}

const void* bh_personality_registry_get_ops(bh_personality_kind_t kind) {
    if (kind <= BH_PERSONALITY_MAX) {
        return personality_table[kind];
    }
    return NULL;
}
