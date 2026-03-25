#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file cuckoo_hash.h
 * @brief Lock-free Cuckoo Hash Table.
 *
 * Designed for fast O(1) thread/process lookup across cores.
 * Optimized via hardware CRC32/CRC64 instructions (x86 `CRC32`, ARM `CRC32`, RISC-V `Zbb`)
 * for high-speed deterministic hashing.
 */

#define CUCKOO_MAX_KICKS 500

/**
 * @struct cuckoo_entry_t
 * @brief Represents an individual hash table entry.
 */
typedef struct {
    uint64_t key;   /**< E.g., Thread ID, Process ID */
    void *value;    /**< Pointer to thread/process control block */
} cuckoo_entry_t;

/**
 * @struct cuckoo_hash_t
 * @brief Represents the cuckoo hash table.
 */
typedef struct {
    cuckoo_entry_t *table[2];  /**< Two separate tables for cuckoo hashing */
    size_t size;               /**< Size of each table */
} cuckoo_hash_t;

/**
 * @brief Initialize the Cuckoo hash table.
 *
 * @param hash Pointer to the hash table.
 * @param size Expected size of the table.
 * @param t1 Pointer to the pre-allocated first table (size elements).
 * @param t2 Pointer to the pre-allocated second table (size elements).
 */
void cuckoo_hash_init(cuckoo_hash_t *hash, size_t size, cuckoo_entry_t *t1, cuckoo_entry_t *t2);

/**
 * @brief Insert a key-value pair into the cuckoo hash table.
 *
 * @param hash Pointer to the cuckoo hash table.
 * @param key The key to insert.
 * @param value The value to associate with the key.
 * @return 0 on success, or an error code (e.g., if max kicks reached).
 */
int cuckoo_hash_insert(cuckoo_hash_t *hash, uint64_t key, void *value);

/**
 * @brief Look up a key in the cuckoo hash table.
 *
 * @param hash Pointer to the cuckoo hash table.
 * @param key The key to look up.
 * @return The associated value, or NULL if not found.
 */
void *cuckoo_hash_lookup(const cuckoo_hash_t *hash, uint64_t key);

/**
 * @brief Remove a key-value pair from the cuckoo hash table.
 *
 * @param hash Pointer to the cuckoo hash table.
 * @param key The key to remove.
 * @return The value that was removed, or NULL if not found.
 */
void *cuckoo_hash_remove(cuckoo_hash_t *hash, uint64_t key);
