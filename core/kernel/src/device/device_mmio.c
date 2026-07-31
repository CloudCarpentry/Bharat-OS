#include "device/device_mmio.h"
#include "kernel.h"
#include "hal/hal_pt.h"
#include "hal/hal_mpa.h"
#include "hal/hal_tlb.h"
#include "mm/physmap.h"

int bh_device_mmio_map(uint64_t phys,
                       size_t size,
                       uint32_t attributes,
                       void **out_virt) {
    if (phys == 0 || size == 0 || !out_virt) {
        return -1;
    }

    if (!physmap_has_linear_map()) {
        *out_virt = (void *)(uintptr_t)phys;
        return 0;
    }

    uint64_t page_offset = phys & 0xFFFU;
    uint64_t aligned_phys = phys & ~0xFFFU;
    uint64_t aligned_size = (size + page_offset + 0xFFFU) & ~0xFFFU;

    void *virt_ptr = physmap_phys_to_virt(phys);
    if (!virt_ptr) {
        return -2;
    }

    uintptr_t base_virt = (uintptr_t)virt_ptr;
    virt_addr_t aligned_virt = (virt_addr_t)base_virt & ~0xFFFU;

    if (active_mem_protect && active_mem_protect->cpu_ops.get_root) {
        phys_addr_t current_root = active_mem_protect->cpu_ops.get_root();
        if (current_root == 0) {
            extern phys_addr_t vmm_get_kernel_root(void);
            current_root = vmm_get_kernel_root();
        }
        if (current_root != 0) {
            phys_addr_t existing_pa = 0;
            size_t mapped_size = 0;
            uint32_t existing_flags = 0;
            if (hal_pt_query_mapping(current_root, aligned_virt, &existing_pa, &mapped_size, &existing_flags) != 0 ||
                existing_pa != aligned_phys) {
                uint32_t flags = HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE | HAL_PT_FLAG_DEVICE | attributes;
                if (hal_pt_map_range(current_root, aligned_virt, aligned_phys, aligned_size, flags) != 0) {
                    return -3;
                }
                hal_tlb_invalidate_all();
            }
        }
    }

    *out_virt = virt_ptr;
    return 0;
}
