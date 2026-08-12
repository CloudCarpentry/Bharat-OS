#ifndef BHARAT_HAL_HMEM_H
#define BHARAT_HAL_HMEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t bh_device_id_t;
typedef int32_t hal_hmem_status_t;

#define HAL_HMEM_OK INT32_C(0)
#define HAL_HMEM_ERR_INVALID INT32_C(-1)
#define HAL_HMEM_ERR_ALREADY_EXISTS INT32_C(-4)
#define HAL_HMEM_ERR_UNSUPPORTED INT32_C(-5)

typedef enum hal_hmem_sync_direction {
  HAL_HMEM_SYNC_TO_CPU = 0,
  HAL_HMEM_SYNC_TO_DEVICE = 1,
  HAL_HMEM_SYNC_BIDIRECTIONAL = 2,
} hal_hmem_sync_direction_t;

typedef struct hal_hmem_device_map_req {
  bh_device_id_t device;
  uintptr_t cpu_address;
  uint64_t length;
  uint32_t access;
  uint32_t reserved;
} hal_hmem_device_map_req_t;

typedef struct hal_hmem_device_map_result {
  uint64_t device_address;
} hal_hmem_device_map_result_t;
typedef struct hal_hmem_device_unmap_req {
  bh_device_id_t device;
  uint64_t device_address;
  uint64_t length;
} hal_hmem_device_unmap_req_t;

typedef struct hal_hmem_ops {
  hal_hmem_status_t (*sync_range)(uintptr_t, size_t, hal_hmem_sync_direction_t);
  hal_hmem_status_t (*map_device)(const hal_hmem_device_map_req_t *,
                                  hal_hmem_device_map_result_t *);
  hal_hmem_status_t (*unmap_device)(const hal_hmem_device_unmap_req_t *);
  bool (*is_cpu_device_coherent)(bh_device_id_t);
} hal_hmem_ops_t;

hal_hmem_status_t hal_hmem_install_ops(const hal_hmem_ops_t *ops);
hal_hmem_status_t hal_hmem_sync_range(uintptr_t addr, size_t length,
                                      hal_hmem_sync_direction_t direction);
hal_hmem_status_t hal_hmem_map_device(const hal_hmem_device_map_req_t *req,
                                      hal_hmem_device_map_result_t *out);
hal_hmem_status_t hal_hmem_unmap_device(const hal_hmem_device_unmap_req_t *req);
bool hal_hmem_is_cpu_device_coherent(bh_device_id_t device);

#endif
