#include "hal/hal_hmem.h"

/* Installed once during boot discovery; immutable afterwards on production
 * systems. */
static const hal_hmem_ops_t *hmem_ops;

hal_hmem_status_t hal_hmem_install_ops(const hal_hmem_ops_t *ops) {
  if (ops == NULL || ops->sync_range == NULL)
    return HAL_HMEM_ERR_INVALID;
  if (__atomic_load_n(&hmem_ops, __ATOMIC_ACQUIRE) != NULL)
    return HAL_HMEM_ERR_ALREADY_EXISTS;
  __atomic_store_n(&hmem_ops, ops, __ATOMIC_RELEASE);
  return HAL_HMEM_OK;
}

hal_hmem_status_t hal_hmem_sync_range(uintptr_t addr, size_t length,
                                      hal_hmem_sync_direction_t direction) {
  const hal_hmem_ops_t *ops = __atomic_load_n(&hmem_ops, __ATOMIC_ACQUIRE);
  if (length == 0U || direction > HAL_HMEM_SYNC_BIDIRECTIONAL)
    return HAL_HMEM_ERR_INVALID;
  if (ops == NULL) {
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    return HAL_HMEM_OK;
  }
  return ops->sync_range(addr, length, direction);
}

hal_hmem_status_t hal_hmem_map_device(const hal_hmem_device_map_req_t *req,
                                      hal_hmem_device_map_result_t *out) {
  const hal_hmem_ops_t *ops = __atomic_load_n(&hmem_ops, __ATOMIC_ACQUIRE);
  if (req == NULL || out == NULL)
    return HAL_HMEM_ERR_INVALID;
  out->device_address = 0U;
  return ops != NULL && ops->map_device != NULL ? ops->map_device(req, out)
                                                : HAL_HMEM_ERR_UNSUPPORTED;
}

hal_hmem_status_t
hal_hmem_unmap_device(const hal_hmem_device_unmap_req_t *req) {
  const hal_hmem_ops_t *ops = __atomic_load_n(&hmem_ops, __ATOMIC_ACQUIRE);
  if (req == NULL)
    return HAL_HMEM_ERR_INVALID;
  return ops != NULL && ops->unmap_device != NULL ? ops->unmap_device(req)
                                                  : HAL_HMEM_ERR_UNSUPPORTED;
}

bool hal_hmem_is_cpu_device_coherent(bh_device_id_t device) {
  const hal_hmem_ops_t *ops = __atomic_load_n(&hmem_ops, __ATOMIC_ACQUIRE);
  return ops != NULL && ops->is_cpu_device_coherent != NULL &&
         ops->is_cpu_device_coherent(device);
}
