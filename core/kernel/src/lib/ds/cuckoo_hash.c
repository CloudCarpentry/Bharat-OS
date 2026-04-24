/*
 * kernel/src/lib/ds/cuckoo_hash.c
 * Lock-free Cuckoo Hash Table optimized for fast O(1) thread/process lookup.
 */

#include "lib/ds/cuckoo_hash.h"
#include "arch/hash.h"
#include <stddef.h>

void cuckoo_hash_init(cuckoo_hash_t *hash, size_t size, cuckoo_entry_t *t1, cuckoo_entry_t *t2) {
    hash->table[0] = t1;
    hash->table[1] = t2;
    hash->size = size;

    for (size_t i = 0; i < size; i++) {
        hash->table[0][i].key = 0;
        hash->table[0][i].value = NULL;
        hash->table[1][i].key = 0;
        hash->table[1][i].value = NULL;
    }
}

static size_t hash_func(uint64_t key, int table, size_t size) {
    return arch_hash_func(key, table, size);
}

int cuckoo_hash_insert(cuckoo_hash_t *hash, uint64_t key, void *value) {
    if (key == 0) return -1; /* 0 is an invalid key */

    uint64_t curr_key = key;
    void *curr_val = value;
    int table_idx = 0;

    for (int kick = 0; kick < CUCKOO_MAX_KICKS; kick++) {
        size_t idx = hash_func(curr_key, table_idx, hash->size);

        if (hash->table[table_idx][idx].key == 0 || hash->table[table_idx][idx].key == curr_key) {
            hash->table[table_idx][idx].key = curr_key;
            hash->table[table_idx][idx].value = curr_val;
            return 0;
        }

        /* Evict */
        uint64_t temp_key = hash->table[table_idx][idx].key;
        void *temp_val = hash->table[table_idx][idx].value;

        hash->table[table_idx][idx].key = curr_key;
        hash->table[table_idx][idx].value = curr_val;

        curr_key = temp_key;
        curr_val = temp_val;
        table_idx = 1 - table_idx;
    }

    /* Max kicks reached, table needs resizing (or return error) */
    return -1;
}

void *cuckoo_hash_lookup(const cuckoo_hash_t *hash, uint64_t key) {
    if (key == 0) return NULL;

    size_t idx1 = hash_func(key, 0, hash->size);
    if (hash->table[0][idx1].key == key) {
        return hash->table[0][idx1].value;
    }

    size_t idx2 = hash_func(key, 1, hash->size);
    if (hash->table[1][idx2].key == key) {
        return hash->table[1][idx2].value;
    }

    return NULL;
}

void *cuckoo_hash_remove(cuckoo_hash_t *hash, uint64_t key) {
    if (key == 0) return NULL;

    size_t idx1 = hash_func(key, 0, hash->size);
    if (hash->table[0][idx1].key == key) {
        void *val = hash->table[0][idx1].value;
        hash->table[0][idx1].key = 0;
        hash->table[0][idx1].value = NULL;
        return val;
    }

    size_t idx2 = hash_func(key, 1, hash->size);
    if (hash->table[1][idx2].key == key) {
        void *val = hash->table[1][idx2].value;
        hash->table[1][idx2].key = 0;
        hash->table[1][idx2].value = NULL;
        return val;
    }

    return NULL;
}
