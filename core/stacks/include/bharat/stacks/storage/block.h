#ifndef BHARAT_STACKS_STORAGE_BLOCK_H
#define BHARAT_STACKS_STORAGE_BLOCK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <bharat/stacks/storage/block_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IO_DEVICE_ROLE_SYSTEM,
    IO_DEVICE_ROLE_DATA,
    IO_DEVICE_ROLE_REMOVABLE,
    IO_DEVICE_ROLE_TEST
} io_device_role_t;

typedef struct {
    io_device_id_t id;
    const char *name;
    const block_driver_ops_t *ops;
    void *driver_context;
    io_device_caps_t caps;
    io_device_state_t state;
    io_device_role_t role;
} block_device_t;

// Compatibility legacy structs
typedef enum {
    BLOCK_REQ_READ,
    BLOCK_REQ_WRITE,
    BLOCK_REQ_FLUSH
} block_req_type_t;

typedef struct {
    block_req_type_t type;
    uint64_t lba;
    uint32_t num_blocks;
    void* buffer;
    int status;
} block_request_t;

typedef struct {
    uint32_t device_id;
    uint32_t block_size;
    uint64_t total_blocks;
} block_device_info_t;

// Registry functions
io_status_t block_driver_register(const block_driver_ops_t *ops);
io_status_t block_driver_unregister(const block_driver_ops_t *ops);

io_status_t block_device_register(block_device_t *device);
io_status_t block_device_unregister(io_device_id_t device_id);
block_device_t *block_device_get(io_device_id_t device_id);
io_status_t block_device_enumerate(io_device_id_t *out_ids, uint32_t capacity, uint32_t *out_count);
io_status_t block_device_find_by_role(io_device_role_t role, io_device_id_t *out_device_id);

// Channel and I/O submission/completion functions
io_status_t block_channel_open(io_device_id_t device_id, const io_channel_config_t *config, void **out_channel);
io_status_t block_channel_close(io_device_id_t device_id, void *channel);

io_status_t block_submit(io_device_id_t device_id, void *channel, const io_request_t *request);
io_status_t block_submit_batch(io_device_id_t device_id, void *channel, const io_request_t *requests, uint32_t request_count, uint32_t *accepted_count);
io_status_t block_poll_completions(io_device_id_t device_id, void *channel, io_completion_t *completions, uint32_t capacity, uint32_t *completion_count);
io_status_t block_cancel(io_device_id_t device_id, void *channel, io_request_id_t request_id);

// Policy helper
io_status_t block_device_resolve_policy(io_device_id_t device_id, io_device_policy_t *out_policy);

// Compatibility legacy functions
int block_queue_request(uint32_t device_id, block_request_t* req);
int block_get_info(uint32_t device_id, block_device_info_t* info);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_STACKS_STORAGE_BLOCK_H */
