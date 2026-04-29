#ifndef BHARAT_KERNEL_DS_BH_REGISTRY_H
#define BHARAT_KERNEL_DS_BH_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/status.h"

/**
 * @file bh_registry.h
 * @brief Boot-time Registry for Kernel Components.
 *
 * This primitive provides a simple, bounded registry for kernel components,
 * primitives, personalities, and other subsystem descriptors.
 *
 * Design:
 * - Dynamic registration during boot using static storage.
 * - Supports lookup by integer ID and optional string name.
 * - No heap allocation (caller provides storage).
 * - "Write-once" semantics: once frozen, no more registrations are allowed.
 * - No unregistration support.
 * - Duplicate ID/name detection.
 */

typedef struct {
    uint32_t id;
    const char *name;
    const void *object;
    uint32_t flags;
} bh_registry_entry_t;

typedef struct {
    bh_registry_entry_t *entries;
    uint32_t capacity;
    uint32_t count;
    bool frozen;
} bh_registry_t;

/**
 * @brief Initialize the registry.
 * @param registry Pointer to the registry structure.
 * @param storage Pointer to the pre-allocated array of registry entries.
 * @param capacity Number of entries the storage can hold.
 * @return K_OK on success.
 */
kstatus_t bh_registry_init(bh_registry_t *registry, bh_registry_entry_t *storage, uint32_t capacity);

/**
 * @brief Register a component.
 * Fails if the registry is frozen, full, or if the ID/name already exists.
 * @param registry Pointer to the registry.
 * @param id Unique integer ID.
 * @param name Optional unique string name (can be NULL).
 * @param object Pointer to the component/descriptor.
 * @param flags Registry flags.
 * @return K_OK on success, K_ERR_ALREADY_EXISTS if ID/name taken,
 *         K_ERR_NO_SPACE if full, K_ERR_BAD_STATE if frozen.
 */
kstatus_t bh_registry_register(bh_registry_t *registry, uint32_t id, const char *name, const void *object, uint32_t flags);

/**
 * @brief Lookup a component by its integer ID.
 * @return Pointer to the object, or NULL if not found.
 */
const void *bh_registry_lookup_id(const bh_registry_t *registry, uint32_t id);

/**
 * @brief Lookup a component by its string name.
 * @return Pointer to the object, or NULL if not found.
 */
const void *bh_registry_lookup_name(const bh_registry_t *registry, const char *name);

/**
 * @brief Freeze the registry. No more registrations will be allowed.
 */
kstatus_t bh_registry_freeze(bh_registry_t *registry);

/**
 * @brief Get the number of registered components.
 */
uint32_t bh_registry_count(const bh_registry_t *registry);

#endif // BHARAT_KERNEL_DS_BH_REGISTRY_H
