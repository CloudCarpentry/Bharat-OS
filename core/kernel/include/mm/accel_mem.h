#ifndef BHARAT_MM_ACCEL_MEM_H
#define BHARAT_MM_ACCEL_MEM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../include/capability.h"
#include "bharat/kernel/ds/bh_refcount.h"

/**
 * Accelerator Memory Policy Flags
 */
typedef uint64_t accel_mem_flags_t;

#define ACCEL_MEM_SHARED    (1ULL << 0)
#define ACCEL_MEM_PINNED    (1ULL << 1)
#define ACCEL_MEM_COHERENT  (1ULL << 2)
#define ACCEL_MEM_STREAMING (1ULL << 3)
#define ACCEL_MEM_SECURE    (1ULL << 4)
#define ACCEL_MEM_MODEL_RO  (1ULL << 5)
#define ACCEL_MEM_TENSOR_RW (1ULL << 6)

typedef enum {
    ACCEL_BUF_STATE_CREATED = 0,
    ACCEL_BUF_STATE_PINNED,
    ACCEL_BUF_STATE_SG_BUILT,
    ACCEL_BUF_STATE_IOVA_BOUND,
    ACCEL_BUF_STATE_MAPPED,
    ACCEL_BUF_STATE_DESTROYED
} accel_buf_state_t;

typedef struct accel_sg_entry {
    uintptr_t phys_addr;
    size_t length;
    size_t offset;
    struct accel_sg_entry *next;
} accel_sg_entry_t;

typedef struct accel_sg_table {
    accel_sg_entry_t *head;
    accel_sg_entry_t *tail;
    size_t num_entries;
    size_t total_length;
} accel_sg_table_t;

struct iommu_domain; // Forward declaration

typedef struct accel_buffer {
    size_t size;
    size_t alignment;
    accel_mem_flags_t flags;

    accel_buf_state_t state;
    uint32_t pin_count;
    bh_refcount_t ref_count;

    uintptr_t *phys_pages;
    size_t num_pages;

    accel_sg_table_t *sg_table;

    struct iommu_domain *iommu_domain;
    uintptr_t iova;

    cap_handle_t owner_cap;

} accel_buffer_t;

int accel_mem_buffer_create(size_t size, size_t alignment, accel_mem_flags_t flags, cap_handle_t owner, accel_buffer_t **out_buf);
void accel_mem_buffer_destroy(accel_buffer_t *buf);

int accel_mem_pin(accel_buffer_t *buf);
int accel_mem_unpin(accel_buffer_t *buf);

int accel_sg_build(accel_buffer_t *buf);
void accel_sg_release(accel_buffer_t *buf);

int accel_iova_bind(accel_buffer_t *buf, struct iommu_domain *domain);
int accel_iova_unbind(accel_buffer_t *buf);

typedef enum {
    ACCEL_SYNC_TO_DEVICE,
    ACCEL_SYNC_FROM_DEVICE,
    ACCEL_SYNC_BIDIRECTIONAL
} accel_sync_dir_t;

int accel_mem_sync(accel_buffer_t *buf, accel_sync_dir_t dir);
void accel_mem_teardown(accel_buffer_t *buf);

#endif // BHARAT_MM_ACCEL_MEM_H
