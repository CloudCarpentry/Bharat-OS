#ifndef BHARAT_ANDROID_PERSONALITY_H
#define BHARAT_ANDROID_PERSONALITY_H

#include <stdint.h>
#include <stddef.h>
#include "personality/personality_types.h"
#include "capability.h"

/*
 * Phase 0: Personality Framework & Ownership Model
 *
 * Defines the Android personality registration and logical object identity.
 * In a multikernel, we do not use raw global kernel pointers.
 * We use logical IDs and home-core authority.
 */

typedef uint32_t android_obj_id_t;

/**
 * @brief Represents the logical identity of an Android-specific kernel object.
 */
typedef struct {
    android_obj_id_t id;          // Logical Object ID
    uint32_t home_core;           // Authoritative core for this object
    uint32_t generation;          // For ABA/use-after-free prevention
    cap_handle_t backing_cap;     // Security/Rights capability handle
} android_logical_obj_t;

/**
 * @brief Android personality descriptor.
 * Attached to a subsystem instance to provide Android semantics.
 */
typedef struct {
    personality_desc_t base;

    // Android-specific global namespace (e.g. Service Manager root)
    android_logical_obj_t service_manager_root;

    // Configuration limits
    uint32_t max_binder_nodes_per_core;
    uint32_t max_ashmem_regions;
} android_personality_t;

/**
 * @brief Initialize and register the Android personality layer.
 */
int android_personality_register(android_personality_t* p);

/**
 * @brief Lookup an Android logical object by ID.
 * Resolves the object and identifies its home core for message routing.
 */
int android_obj_lookup(android_obj_id_t id, android_logical_obj_t* out_obj);

#endif // BHARAT_ANDROID_PERSONALITY_H
