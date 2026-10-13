#include "../../include/mm/physmap.h"
#include "../../include/kernel.h"
#include "kernel/status.h"
#include <stddef.h>

void physmap_init(void) {
    // Basic initialization for the linear map boundaries.
}

bool physmap_has_linear_map(void) {
    const hal_translate_ops_t* ops = hal_translate_ops();
    if (!ops || !ops->has_linear_physmap) return false;
    return ops->has_linear_physmap();
}

void *physmap_phys_to_virt(phys_addr_t phys) {
    const hal_translate_ops_t* ops = hal_translate_ops();
    // For MMU-Lite, this might panic or return NULL if outside permanent window
    // For now, we just pass to the backend ops
    if (!ops || !ops->phys_to_virt) return NULL;
    return ops->phys_to_virt(phys);
}

phys_addr_t physmap_virt_to_phys(const void *virt) {
    const hal_translate_ops_t* ops = hal_translate_ops();
    if (!ops || !ops->virt_to_phys) return 0;
    return ops->virt_to_phys(virt);
}

translate_backend_kind_t physmap_backend_type(void) {
    const hal_translate_ops_t* ops = hal_translate_ops();
    if (!ops || !ops->backend_type) return TRANSLATE_BACKEND_NONE;
    return ops->backend_type();
}

translate_exec_class_t physmap_exec_class(void) {
    const hal_translate_ops_t* ops = hal_translate_ops();
    // Defaulting to MMU_FULL if undefined, although backend should provide it
    if (!ops || !ops->exec_class) return TRANSLATE_EXEC_MMU_FULL;
    return ops->exec_class();
}

int mm_memset_phys_range(phys_addr_t phys, uint8_t value, size_t size) {
    if (size == 0U) {
        return K_OK;
    }

    if (phys + size < phys) {
        return K_ERR_INVALID_ARG;
    }

    /* Validate that the entire range is mapped by checking page by page.
     * This avoids partial modification if a subsequent page is unmapped. */
    phys_addr_t current_phys = phys;
    size_t remaining = size;
    while (remaining > 0) {
        size_t page_offset = current_phys & (PAGE_SIZE - 1);
        size_t chunk = PAGE_SIZE - page_offset;
        if (chunk > remaining) {
            chunk = remaining;
        }

        if (!physmap_phys_to_virt(current_phys)) {
            return K_ERR_VM_UNMAPPED;
        }

        current_phys += chunk;
        remaining -= chunk;
    }

    /* Range is fully valid. Now perform the actual write. */
    current_phys = phys;
    remaining = size;
    while (remaining > 0) {
        size_t page_offset = current_phys & (PAGE_SIZE - 1);
        size_t chunk = PAGE_SIZE - page_offset;
        if (chunk > remaining) {
            chunk = remaining;
        }

        uint8_t *dst = (uint8_t *)physmap_phys_to_virt(current_phys);
        for (size_t i = 0; i < chunk; i++) {
            dst[i] = value;
        }

        current_phys += chunk;
        remaining -= chunk;
    }

    return K_OK;
}

int mm_zero_phys_range(phys_addr_t phys, size_t size) {
    return mm_memset_phys_range(phys, 0U, size);
}
