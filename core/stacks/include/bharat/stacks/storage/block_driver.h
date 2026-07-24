#ifndef BHARAT_STACKS_STORAGE_BLOCK_DRIVER_H
#define BHARAT_STACKS_STORAGE_BLOCK_DRIVER_H

#include <bharat/io/block.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    IO_MEDIA_HDD = 1,
    IO_MEDIA_SSD_SATA,
    IO_MEDIA_SSD_NVME,
    IO_MEDIA_EMMC,
    IO_MEDIA_SD,
    IO_MEDIA_RAM,
    IO_MEDIA_FLASH_NOR
} io_media_class_t;

typedef enum {
    IO_TRANSPORT_PCI = 1,
    IO_TRANSPORT_VIRTIO,
    IO_TRANSPORT_SPI,
    IO_TRANSPORT_SDIO,
    IO_TRANSPORT_RAM,
    IO_TRANSPORT_MEM
} io_transport_t;

typedef struct {
    io_media_class_t media_class;
    io_transport_t transport;

    uint32_t logical_block_size;
    uint32_t physical_block_size;
    uint32_t optimal_io_size;
    uint32_t erase_block_size;

    uint64_t capacity_bytes;

    uint32_t max_transfer_bytes;
    uint16_t max_segments;
    uint16_t max_hw_queues;
    uint16_t max_queue_depth;
    uint16_t interrupt_vectors;

    bool rotational;
    bool removable;
    bool read_only;

    bool dma_supported;
    bool dma_coherent;
    bool scatter_gather_supported;

    bool flush_supported;
    bool fua_supported;
    bool discard_supported;
    bool write_zeroes_supported;

    bool volatile_write_cache;
    bool power_loss_protection;
    bool polling_supported;
    bool hybrid_completion_supported;
} io_device_caps_t;

typedef enum {
    IO_QUEUE_POLICY_SINGLE_OWNER,
    IO_QUEUE_POLICY_SHARED_MP,
    IO_QUEUE_POLICY_PER_CORE
} io_queue_policy_t;

typedef enum {
    IO_COMPLETION_POLICY_INTERRUPT,
    IO_COMPLETION_POLICY_POLL,
    IO_COMPLETION_POLICY_HYBRID
} io_completion_policy_t;

typedef enum {
    IO_CACHE_POLICY_NONE,
    IO_CACHE_POLICY_FIXED,
    IO_CACHE_POLICY_WRITEBACK,
    IO_CACHE_POLICY_NUMA
} io_cache_policy_t;

typedef enum {
    IO_DURABILITY_POLICY_LAZY,
    IO_DURABILITY_POLICY_STRICT_FUA,
    IO_DURABILITY_POLICY_WRITE_THROUGH
} io_durability_policy_t;

typedef enum {
    IO_POWER_POLICY_PERFORMANCE,
    IO_POWER_POLICY_BALANCED,
    IO_POWER_POLICY_POWERSAVE
} io_power_policy_t;

typedef enum {
    IO_DEV_STATE_UNINITIALIZED = 0,
    IO_DEV_STATE_READY,
    IO_DEV_STATE_BUSY,
    IO_DEV_STATE_ERROR,
    IO_DEV_STATE_REMOVED
} io_device_state_t;

typedef struct {
    io_device_id_t device_id;

    io_queue_policy_t queue_policy;
    io_completion_policy_t completion_policy;
    io_cache_policy_t cache_policy;
    io_durability_policy_t durability_policy;
    io_power_policy_t power_policy;

    uint16_t queue_count;
    uint16_t queue_depth;
    uint32_t max_transfer_bytes;
    uint64_t cache_budget_bytes;

    bool allow_direct_io;
    bool allow_request_merge;
    bool allow_readahead;
} io_device_policy_t;

typedef struct {
    uint16_t queue_id;
    uint16_t queue_depth;
    bool enable_polling;
} io_channel_config_t;

typedef struct block_driver_ops {
    io_status_t (*probe)(void *driver_ctx);
    io_status_t (*start)(void *driver_ctx);
    io_status_t (*stop)(void *driver_ctx);

    io_status_t (*get_caps)(
        void *driver_ctx,
        io_device_caps_t *out_caps);

    io_status_t (*open_channel)(
        void *driver_ctx,
        const io_channel_config_t *config,
        void **out_channel);

    io_status_t (*close_channel)(
        void *channel);

    io_status_t (*submit)(
        void *channel,
        const io_request_t *request);

    io_status_t (*submit_batch)(
        void *channel,
        const io_request_t *requests,
        uint32_t request_count,
        uint32_t *accepted_count);

    io_status_t (*poll_completions)(
        void *channel,
        io_completion_t *completions,
        uint32_t capacity,
        uint32_t *completion_count);

    io_status_t (*cancel)(
        void *channel,
        io_request_id_t request_id);

    io_status_t (*flush)(
        void *channel,
        io_request_id_t request_id);

    io_status_t (*discard)(
        void *channel,
        uint64_t lba,
        uint32_t block_count,
        io_request_id_t request_id);

    io_status_t (*reset)(
        void *driver_ctx);
} block_driver_ops_t;

#endif /* BHARAT_STACKS_STORAGE_BLOCK_DRIVER_H */
