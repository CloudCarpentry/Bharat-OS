#ifndef BHARAT_LIB_HANDLE_TABLE_H
#define BHARAT_LIB_HANDLE_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BH_HANDLE_TABLE_SUCCESS 0
#define BH_HANDLE_TABLE_ERR_INVALID -1
#define BH_HANDLE_TABLE_ERR_NO_MEM -2
#define BH_HANDLE_TABLE_ERR_NOT_FOUND -3
#define BH_HANDLE_TABLE_ERR_WRONG_TYPE -4

typedef uint64_t bh_handle_t;

#define BH_HANDLE_INVALID 0

typedef struct {
    uint32_t generation;
    uint32_t type;
    void *object;
    uint64_t rights;
    bool active;
} bh_user_handle_slot_t;

typedef struct {
    bh_user_handle_slot_t *slots;
    size_t capacity;
    size_t count;
} bh_user_handle_table_t;

int bh_user_handle_table_init(bh_user_handle_table_t *table, void *storage, size_t capacity);
int bh_user_handle_alloc(bh_user_handle_table_t *table, void *object, uint32_t type, uint64_t rights, bh_handle_t *out_handle);
int bh_user_handle_lookup(const bh_user_handle_table_t *table, bh_handle_t handle, uint32_t expected_type, void **out_object, uint64_t *out_rights);
int bh_user_handle_revoke(bh_user_handle_table_t *table, bh_handle_t handle);
bool bh_user_handle_validate(const bh_user_handle_table_t *table, bh_handle_t handle);

static inline bh_handle_t bh_user_handle_make(uint32_t index, uint32_t generation) {
    return ((uint64_t)generation << 32) | index;
}

static inline uint32_t bh_user_handle_index(bh_handle_t handle) {
    return (uint32_t)(handle & 0xFFFFFFFF);
}

static inline uint32_t bh_user_handle_generation(bh_handle_t handle) {
    return (uint32_t)(handle >> 32);
}

#endif // BHARAT_LIB_HANDLE_TABLE_H
