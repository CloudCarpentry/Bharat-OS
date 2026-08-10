#include "handle_table.h"

// Custom basic memset implementation for freestanding use
static void handle_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
}

int bh_user_handle_table_init(bh_user_handle_table_t *table, void *storage, size_t capacity) {
    if (!table || !storage || capacity == 0) {
        return BH_HANDLE_TABLE_ERR_INVALID;
    }
    table->slots = (bh_user_handle_slot_t *)storage;
    table->capacity = capacity;
    table->count = 0;
    handle_memset(storage, 0, capacity * sizeof(bh_user_handle_slot_t));
    return BH_HANDLE_TABLE_SUCCESS;
}

int bh_user_handle_alloc(bh_user_handle_table_t *table, void *object, uint32_t type, uint64_t rights, bh_handle_t *out_handle) {
    if (!table || !object || !out_handle) {
        return BH_HANDLE_TABLE_ERR_INVALID;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        if (!table->slots[i].active) {
            table->slots[i].active = true;
            table->slots[i].object = object;
            table->slots[i].type = type;
            table->slots[i].rights = rights;

            // Increment generation to invalidate old handles to this slot
            table->slots[i].generation++;
            if (table->slots[i].generation == 0) {
                table->slots[i].generation = 1; // Generation 0 is never issued
            }

            *out_handle = bh_user_handle_make((uint32_t)i, table->slots[i].generation);
            table->count++;
            return BH_HANDLE_TABLE_SUCCESS;
        }
    }

    return BH_HANDLE_TABLE_ERR_NO_MEM;
}

int bh_user_handle_lookup(const bh_user_handle_table_t *table, bh_handle_t handle, uint32_t expected_type, void **out_object, uint64_t *out_rights) {
    if (!table || !out_object || handle == BH_HANDLE_INVALID) {
        return BH_HANDLE_TABLE_ERR_INVALID;
    }

    uint32_t index = bh_user_handle_index(handle);
    uint32_t generation = bh_user_handle_generation(handle);

    if (index >= table->capacity) {
        return BH_HANDLE_TABLE_ERR_INVALID;
    }

    bh_user_handle_slot_t *slot = &table->slots[index];

    if (!slot->active || slot->generation != generation) {
        return BH_HANDLE_TABLE_ERR_NOT_FOUND; // Stale or invalid handle
    }

    if (expected_type != 0 && slot->type != expected_type) {
        return BH_HANDLE_TABLE_ERR_WRONG_TYPE;
    }

    *out_object = slot->object;
    if (out_rights) {
        *out_rights = slot->rights;
    }

    return BH_HANDLE_TABLE_SUCCESS;
}

int bh_user_handle_revoke(bh_user_handle_table_t *table, bh_handle_t handle) {
    if (!table || handle == BH_HANDLE_INVALID) {
        return BH_HANDLE_TABLE_ERR_INVALID;
    }

    uint32_t index = bh_user_handle_index(handle);
    uint32_t generation = bh_user_handle_generation(handle);

    if (index >= table->capacity) {
        return BH_HANDLE_TABLE_ERR_INVALID;
    }

    bh_user_handle_slot_t *slot = &table->slots[index];

    if (!slot->active || slot->generation != generation) {
        return BH_HANDLE_TABLE_ERR_NOT_FOUND;
    }

    slot->active = false;
    slot->object = NULL;
    table->count--;

    return BH_HANDLE_TABLE_SUCCESS;
}

bool bh_user_handle_validate(const bh_user_handle_table_t *table, bh_handle_t handle) {
    if (!table || handle == BH_HANDLE_INVALID) {
        return false;
    }

    uint32_t index = bh_user_handle_index(handle);
    uint32_t generation = bh_user_handle_generation(handle);

    if (index >= table->capacity) {
        return false;
    }

    bh_user_handle_slot_t *slot = &table->slots[index];
    return slot->active && slot->generation == generation;
}
