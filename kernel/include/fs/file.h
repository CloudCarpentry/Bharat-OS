#ifndef BHARAT_FS_FILE_H
#define BHARAT_FS_FILE_H

#include "fs/vfs.h"
#include "advanced/formal_verif.h"

/*
 * vfs_file_t: Represents an open file description/handle.
 * Governs negotiated read/write rights using a capability-based live token
 * to prevent ambient privilege escalation.
 */
typedef struct vfs_file {
    int in_use;
    int flags;          // O_RDONLY, O_WRONLY, O_RDWR, etc.
    uint64_t offset;    // Current seek offset

    // The canonical node this file points to
    vfs_node_t* node;

    // Live capability token governing this open handle's specific rights
    capability_t handle_cap;

} vfs_file_t;

// Capability-checked file operations (neutral semantics, not POSIX)
int vfs_open_file(const char* path, int flags, capability_t* caller_cap, int* out_fd);
int vfs_read_file(int fd, void* buffer, size_t size, capability_t* caller_cap);
int vfs_write_file(int fd, const void* buffer, size_t size, capability_t* caller_cap);
int vfs_close_file(int fd, capability_t* caller_cap);

#ifdef TESTING
void vfs_file_test_reset_state(void);
#endif

#endif // BHARAT_FS_FILE_H