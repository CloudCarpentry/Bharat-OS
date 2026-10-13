#ifndef BHARAT_MM_HMEM_H
#define BHARAT_MM_HMEM_H

#include "hal/hal_hmem.h"
#include "kernel/status.h"
#include "mm/pmm.h"
#include "mm/vm_object.h"
#include "mm/aspace.h"
#include "spinlock.h"
#include <bharat/memory/hmem.h>
#include <stdint.h>
#include <stdbool.h>

#define BH_HMEM_MAX_DEVICE_MAPPINGS 4U

typedef enum bh_hmem_state {
  BH_HMEM_LIVE = 0,
  BH_HMEM_DYING = 1,
  BH_HMEM_DEAD = 2
} bh_hmem_state_t;

typedef struct bh_hmem_device_mapping {
  bh_device_id_t device;
  uint64_t device_address;
  uint64_t visible_generation;
  uint32_t access;
  uint32_t flags;
} bh_hmem_device_mapping_t;

/*
 * Kernel HMEM Object representation.
 * Synchronization and Ownership Model:
 *  - Each HMEM object has an authoritative owner core (`owner_core_id`).
 *  - Mutable state transitions are serialized under `lock` (spinlock).
 *  - Backed by contiguous physical frames allocated from PMM (`pmm_block`).
 *  - Kernel-mode mapping is available via `kernel_mapping` (physmap).
 *  - Userspace mapping attaches `vm_obj` to calling process's `address_space_t`
 *    at `mapped_user_va`.
 */
typedef struct bh_hmem {
  uint64_t object_id;
  uint32_t generation;
  uint32_t owner_core_id;
  uint32_t slot;
  uint64_t size;
  uint64_t alignment;
  uint64_t usage_flags;
  uint64_t property_flags;
  uint64_t rights;
  uint32_t preferred_domain;
  uint32_t cpu_map_count;
  uintptr_t mapped_user_va;       /* User virtual address if mapped */
  address_space_t *mapped_aspace; /* Target user address space if mapped */
  pmm_block_t pmm_block;          /* Backing contiguous physical frames */
  void *kernel_mapping;           /* Kernel direct-map virtual address */
  vm_object_t *vm_obj;            /* VM object for region attachment */
  uint64_t content_generation;
  bh_hmem_state_t state;
  spinlock_t lock;
  bh_hmem_device_mapping_t mappings[BH_HMEM_MAX_DEVICE_MAPPINGS];
} bh_hmem_t;

/* Kernel-internal lifecycle and mapping functions */
kstatus_t bh_hmem_create(const bharat_hmem_desc_v1_t *desc, uint64_t rights,
                         bharat_hmem_handle_t *out);
kstatus_t bh_hmem_create_object(const bharat_hmem_desc_v1_t *desc, uint64_t rights,
                                uint32_t owner_core, bh_hmem_t **out_obj);
kstatus_t bh_hmem_destroy(bharat_hmem_handle_t handle);
kstatus_t bh_hmem_destroy_object(bh_hmem_t *object);
kstatus_t bh_hmem_lookup(bharat_hmem_handle_t handle, bh_hmem_t **out);
kstatus_t bh_hmem_get_info(bharat_hmem_handle_t handle,
                           bharat_hmem_info_v1_t *out);
kstatus_t bh_hmem_map_cpu(bharat_hmem_handle_t handle, uint32_t access,
                          uintptr_t *out_addr);
kstatus_t bh_hmem_unmap_cpu(bharat_hmem_handle_t handle);
kstatus_t bh_hmem_sync_for_cpu(bharat_hmem_handle_t handle, uint64_t offset,
                               uint64_t length);
kstatus_t bh_hmem_sync_for_device(bharat_hmem_handle_t handle, uint64_t offset,
                                  uint64_t length);
kstatus_t bh_hmem_map_device(bharat_hmem_handle_t handle, bh_device_id_t device,
                             uint32_t access, uint64_t *device_addr);
kstatus_t bh_hmem_unmap_device(bharat_hmem_handle_t handle,
                               bh_device_id_t device);

#endif /* BHARAT_MM_HMEM_H */
