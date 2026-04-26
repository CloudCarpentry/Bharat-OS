#include <bharat/kernel/ds/bh_handle_table.h>
#include <string.h>

kstatus_t bh_handle_table_init(bh_handle_table_t *table, void *storage, size_t capacity) {
    if (!table || !storage || capacity == 0) {
        return K_ERR_INVALID_ARG;
    }
    table->slots = (bh_handle_slot_t *)storage;
    table->capacity = capacity;
    table->count = 0;
    memset(storage, 0, capacity * sizeof(bh_handle_slot_t));
    return K_OK;
}

kstatus_t bh_handle_alloc(bh_handle_table_t *table, void *object, uint32_t type, uint64_t rights, bh_handle_t *out_handle) {
    if (!table || !object || !out_handle) {
        return K_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        if (!table->slots[i].active) {
            table->slots[i].active = true;
            table->slots[i].object = object;
            table->slots[i].type = type;
            table->slots[i].rights = rights;
            // Increment generation to invalidate old handles to this slot
            table->slots[i].generation++;

            *out_handle = bh_handle_make((uint32_t)i, table->slots[i].generation);
            table->count++;
            return K_OK;
        }
    }

    return K_ERR_NO_MEMORY; // Or K_ERR_LIMIT_EXCEEDED
}

kstatus_t bh_handle_lookup(const bh_handle_table_t *table, bh_handle_t handle, uint32_t expected_type, void **out_object, uint64_t *out_rights) {
    if (!table || !out_object) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t index = bh_handle_index(handle);
    uint32_t generation = bh_handle_generation(handle);

    if (index >= table->capacity) {
        return K_ERR_INVALID_ARG;
    }

    bh_handle_slot_t *slot = &table->slots[index];

    if (!slot->active || slot->generation != generation) {
        return K_ERR_NOT_FOUND; // Stale or invalid handle
    }

    if (expected_type != 0 && slot->type != expected_type) {
        return K_ERR_CAP_WRONG_TYPE;
    }

    *out_object = slot->object;
    if (out_rights) {
        *out_rights = slot->rights;
    }

    return K_OK;
}

kstatus_t bh_handle_revoke(bh_handle_table_t *table, bh_handle_t handle) {
    if (!table) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t index = bh_handle_index(handle);
    uint32_t generation = bh_handle_generation(handle);

    if (index >= table->capacity) {
        return K_ERR_INVALID_ARG;
    }

    bh_handle_slot_t *slot = &table->slots[index];

    if (!slot->active || slot->generation != generation) {
        return K_ERR_NOT_FOUND;
    }

    slot->active = false;
    slot->object = NULL;
    table->count--;

    return K_OK;
}

bool bh_handle_validate(const bh_handle_table_t *table, bh_handle_t handle) {
    if (!table) {
        return false;
    }

    uint32_t index = bh_handle_index(handle);
    uint32_t generation = bh_handle_generation(handle);

    if (index >= table->capacity) {
        return false;
    }

    bh_handle_slot_t *slot = &table->slots[index];
    return slot->active && slot->generation == generation;
}
