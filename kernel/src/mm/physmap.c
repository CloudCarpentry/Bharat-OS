#include "../../include/mm/physmap.h"
#include "../../include/kernel.h"
#include <stddef.h>

// Architecture-specific offset, defined in hal/xxx/hal_pt_xxx.c or similar hardware headers
extern const virt_addr_t g_kernel_virt_offset;

// High-half boundaries and sizes. For now, we assume a simple 1:1 offset map.
// In future phases, these bounds will handle sparse maps and different granularities.
extern const size_t g_kernel_physmap_size;

void physmap_init(void) {
    // Basic initialization for the linear map boundaries.
    // By default, architectures define KERNEL_VIRT_OFFSET directly, and we just link to it.
}

void *phys_to_virt_linear(phys_addr_t pa) {
    // If we have proper bounds checking, we check it here:
    // if (!phys_is_linearly_mapped(pa)) return NULL;
    return (void *)((uintptr_t)pa + g_kernel_virt_offset);
}

phys_addr_t virt_to_phys_linear(const void *va) {
    if (!virt_is_in_linear_map(va)) return 0;
    return (phys_addr_t)((uintptr_t)va - g_kernel_virt_offset);
}

bool virt_is_in_linear_map(const void *va) {
    virt_addr_t addr = (virt_addr_t)(uintptr_t)va;
    // Typical check for high-half addresses >= KERNEL_VIRT_OFFSET
    return addr >= g_kernel_virt_offset;
}

bool phys_is_linearly_mapped(phys_addr_t pa) {
    (void)pa;
    // For standard baseline kernels, all RAM is linearly mapped.
    // This will become sparse-aware in future iterations.
    return true;
}
