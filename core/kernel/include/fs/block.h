#ifndef BHARAT_FS_BLOCK_H
#define BHARAT_FS_BLOCK_H

#warning "This header is legacy/deprecated. Prevent new code from including this. Use <bharat/io/block.h> or <bharat/stacks/storage/block.h> instead."

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Legacy types marked deprecated with non-clashing names
typedef enum {
    BLOCK_REQ_READ_DEPRECATED,
    BLOCK_REQ_WRITE_DEPRECATED,
    BLOCK_REQ_FLUSH_DEPRECATED
} block_req_type_legacy_t __attribute__((deprecated("Use io_opcode_t instead")));

typedef struct {
    block_req_type_legacy_t type;
    uint64_t lba;
    uint32_t num_blocks;
    void* buffer;
    int status;
} block_request_legacy_t __attribute__((deprecated("Use io_request_t instead")));

typedef struct {
    uint32_t device_id;
    uint32_t block_size;
    uint64_t total_blocks;
} block_device_info_legacy_t __attribute__((deprecated("Use io_device_caps_t instead")));

#ifdef __cplusplus
}
#endif

#endif // BHARAT_FS_BLOCK_H
