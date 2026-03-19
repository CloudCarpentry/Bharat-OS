#ifndef BHARAT_FS_SERVICE_H
#define BHARAT_FS_SERVICE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Subsystem Contract: File System Service
 *
 * Defines the user-space contract and wire formats for communicating with
 * the `services/file_system` daemon over IPC/URPC.
 */

#define FS_OP_MOUNT   0x01
#define FS_OP_OPEN    0x02
#define FS_OP_CLOSE   0x03
#define FS_OP_READ    0x04
#define FS_OP_WRITE   0x05
#define FS_OP_STAT    0x06
#define FS_OP_READDIR 0x07

// Basic file system IPC response structure
typedef struct {
    int32_t status;        // 0 on success, < 0 on error (e.g., -ENOENT)
    uint32_t bytes_xfer;   // Bytes read or written
    uint32_t object_id;    // Handle returned by open
} fs_response_t;

// Open request
typedef struct {
    uint32_t opcode;       // FS_OP_OPEN
    uint32_t flags;        // VFS_OPEN_READ, VFS_OPEN_WRITE
    char path[256];
} fs_open_req_t;

// Read request (URPC bound for larger transfers)
typedef struct {
    uint32_t opcode;       // FS_OP_READ
    uint32_t object_id;
    uint64_t offset;
    uint32_t size;
} fs_read_req_t;

// Write request (URPC bound for larger transfers)
typedef struct {
    uint32_t opcode;       // FS_OP_WRITE
    uint32_t object_id;
    uint64_t offset;
    uint32_t size;
} fs_write_req_t;

// Close request
typedef struct {
    uint32_t opcode;       // FS_OP_CLOSE
    uint32_t object_id;
} fs_close_req_t;

#endif // BHARAT_FS_SERVICE_H
