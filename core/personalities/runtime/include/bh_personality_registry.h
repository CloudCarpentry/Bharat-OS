#ifndef BH_PERSONALITY_REGISTRY_H
#define BH_PERSONALITY_REGISTRY_H

#include "bh_personality.h"
#include "personality_ops.h" // Assuming this contains bh_personality_ops_t

/**
 * @brief Register a personality with the runtime.
 */
void bh_personality_registry_register(bh_personality_kind_t kind, const void *ops);

/**
 * @brief Get the operations for a given personality kind.
 */
const void* bh_personality_registry_get_ops(bh_personality_kind_t kind);

#endif // BH_PERSONALITY_REGISTRY_H
