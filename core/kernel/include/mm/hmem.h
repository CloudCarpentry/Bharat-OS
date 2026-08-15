#ifndef BHARAT_MM_HMEM_H
#define BHARAT_MM_HMEM_H

#include "hal/hal_hmem.h"
#include "kernel/status.h"
#include <bharat/memory/hmem.h>
#include <stdint.h>

#define BH_HMEM_MAX_DEVICE_MAPPINGS 4U

typedef enum bh_hmem_state {
  BH_HMEM_LIVE = 0,
  BH_HMEM_DYING = 1,
  BH_HMEM_DEAD = 2
} bh_hmem_state_t;

kstatus_t bh_hmem_create(const bharat_hmem_desc_v1_t *desc, uint64_t rights,
                         bharat_hmem_handle_t *out);
kstatus_t bh_hmem_destroy(bharat_hmem_handle_t handle);
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

#endif
