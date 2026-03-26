#ifndef BHARAT_HAL_MEM_MODEL_H
#define BHARAT_HAL_MEM_MODEL_H

#include <stdint.h>
#include <stddef.h>
#include "../mm.h"

typedef enum {
    HAL_MEM_MODEL_NONE,
    HAL_MEM_MODEL_MPU,
    HAL_MEM_MODEL_MMU_LITE,
    HAL_MEM_MODEL_MMU
} hal_mem_model_t;

typedef struct hal_mem_backend_ops {
    int (*map)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags);
    int (*unmap)(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, phys_addr_t *unmapped_paddr);
    int (*protect)(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags);
    int (*activate)(phys_addr_t root_pt);
} hal_mem_backend_ops_t;

const hal_mem_backend_ops_t* hal_mem_get_backend(void);
hal_mem_model_t hal_mem_get_model(void);
void hal_mem_set_backend(hal_mem_model_t model, hal_mem_backend_ops_t *backend);

#endif // BHARAT_HAL_MEM_MODEL_H
