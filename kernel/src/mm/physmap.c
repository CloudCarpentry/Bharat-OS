#include "../../include/mm/physmap.h"
#include "../../include/kernel.h"
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
