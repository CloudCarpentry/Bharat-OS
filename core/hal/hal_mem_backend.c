#include "hal/hal_mem_model.h"

// Define a null backend for initialization/failsafe
static int null_map(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)size; (void)flags;
    return -1;
}

static int null_unmap(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, phys_addr_t *unmapped_paddr) {
    (void)root_pt; (void)vaddr; (void)size; (void)unmapped_paddr;
    return -1;
}

static int null_protect(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    (void)root_pt; (void)vaddr; (void)size; (void)new_flags;
    return -1;
}

static int null_activate(phys_addr_t root_pt) {
    (void)root_pt;
    return -1;
}

static hal_mem_backend_ops_t active_mem_backend = {
    .map = null_map,
    .unmap = null_unmap,
    .protect = null_protect,
    .activate = null_activate
};

static hal_mem_model_t active_mem_model = HAL_MEM_MODEL_NONE;

const hal_mem_backend_ops_t* hal_mem_get_backend(void) {
    return &active_mem_backend;
}

hal_mem_model_t hal_mem_get_model(void) {
    return active_mem_model;
}

void hal_mem_set_backend(hal_mem_model_t model, hal_mem_backend_ops_t *backend) {
    if (backend) {
        active_mem_backend = *backend;
        active_mem_model = model;
    }
}
