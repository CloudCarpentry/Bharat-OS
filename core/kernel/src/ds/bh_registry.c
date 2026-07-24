#include "bharat/kernel/ds/bh_registry.h"
#include "lib/base/string.h"

kstatus_t bh_registry_init(bh_registry_t *registry, bh_registry_entry_t *storage, uint32_t capacity) {
    if (!registry || !storage || capacity == 0) {
        return K_ERR_INVALID_ARG;
    }

    registry->entries = storage;
    registry->capacity = capacity;
    registry->count = 0;
    registry->frozen = false;

    return K_OK;
}

kstatus_t bh_registry_register(bh_registry_t *registry, uint32_t id, const char *name, const void *object, uint32_t flags) {
    if (!registry) return K_ERR_INVALID_ARG;
    if (registry->frozen) return K_ERR_BAD_STATE;
    if (registry->count >= registry->capacity) return K_ERR_NO_MEMORY; // Using K_ERR_NO_MEMORY as K_ERR_NO_SPACE is not in status.h

    /* Check for duplicates */
    for (uint32_t i = 0; i < registry->count; i++) {
        if (registry->entries[i].id == id) {
            return K_ERR_ALREADY_EXISTS;
        }
        if (name && registry->entries[i].name && strcmp(registry->entries[i].name, name) == 0) {
            return K_ERR_ALREADY_EXISTS;
        }
    }

    /* Add new entry */
    registry->entries[registry->count].id = id;
    registry->entries[registry->count].name = name;
    registry->entries[registry->count].object = object;
    registry->entries[registry->count].flags = flags;
    registry->count++;

    return K_OK;
}

const void *bh_registry_lookup_id(const bh_registry_t *registry, uint32_t id) {
    if (!registry) return NULL;

    for (uint32_t i = 0; i < registry->count; i++) {
        if (registry->entries[i].id == id) {
            return registry->entries[i].object;
        }
    }
    return NULL;
}

const void *bh_registry_lookup_name(const bh_registry_t *registry, const char *name) {
    if (!registry || !name) return NULL;

    for (uint32_t i = 0; i < registry->count; i++) {
        if (registry->entries[i].name && strcmp(registry->entries[i].name, name) == 0) {
            return registry->entries[i].object;
        }
    }
    return NULL;
}

kstatus_t bh_registry_freeze(bh_registry_t *registry) {
    if (!registry) return K_ERR_INVALID_ARG;
    registry->frozen = true;
    return K_OK;
}

uint32_t bh_registry_count(const bh_registry_t *registry) {
    return registry ? registry->count : 0;
}
