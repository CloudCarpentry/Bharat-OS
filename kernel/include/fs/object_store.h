#ifndef BHARAT_FS_OBJECT_STORE_H
#define BHARAT_FS_OBJECT_STORE_H

#include "fs/vfs.h"
#include "advanced/formal_verif.h"

/*
 * Blob/object storage interfaces.
 * Utilizes URI-based addressing (e.g., /blob/remote/bucket/key)
 * Designed for immutable reads and staged, transactional writes.
 */

// Represents an object stored in a blob backend
typedef struct object_store {
    char uri[256];
    uint64_t size;
    uint32_t flags; // E.g., read-only, staged

    // Content hash/ETag for data integrity checks
    char etag[64];

    // Read an object (potentially streaming or chunked)
    int (*get_object)(struct object_store* obj, void* buffer, size_t size, uint64_t offset, capability_t* cap);

    // Staged write for an object (atomic commit flows)
    int (*put_object)(struct object_store* obj, const void* buffer, size_t size, uint64_t offset, capability_t* cap);

    // Commit a staged object to its permanent URI
    int (*commit_object)(struct object_store* temp_obj, const char* final_uri, capability_t* cap);

} object_store_t;

// Register a blob backend/gateway
int object_store_register(object_store_t* store, capability_t* cap);

// Look up a blob object by URI
int object_store_lookup(const char* uri, object_store_t** out_store, capability_t* cap);

#endif // BHARAT_FS_OBJECT_STORE_H