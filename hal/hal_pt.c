#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/kernel.h"
#include <stdbool.h>
#include <stddef.h>

hal_pt_ops_t *active_hal_pt = NULL;
hal_tlb_ops_t *active_hal_tlb = NULL;

static inline bool is_page_aligned_u64(uint64_t val) {
    return (val & (PAGE_SIZE - 1U)) == 0U;
}

#if defined(__x86_64__) || defined(_M_X64)
extern hal_pt_ops_t x86_hal_pt_ops;
extern hal_tlb_ops_t x86_hal_tlb_ops;
#elif defined(__aarch64__) || defined(_M_ARM64)
extern hal_pt_ops_t arm64_hal_pt_ops;
extern hal_tlb_ops_t arm64_hal_tlb_ops;
#elif defined(__arm__)
extern hal_pt_ops_t arm32_hal_pt_ops;
extern hal_tlb_ops_t arm32_hal_tlb_ops;
#elif defined(__riscv) && __riscv_xlen == 64
extern hal_pt_ops_t riscv64_hal_pt_ops;
extern hal_tlb_ops_t riscv64_hal_tlb_ops;
#elif defined(__riscv) && __riscv_xlen == 32
extern hal_pt_ops_t riscv32_hal_pt_ops;
extern hal_tlb_ops_t riscv32_hal_tlb_ops;
#endif

#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(PROFILE_MPU_ONLY)
extern hal_pt_ops_t mpu_hal_pt_ops;
extern hal_tlb_ops_t mpu_hal_tlb_ops;
#endif

void hal_pt_init(void) {
#if defined(__x86_64__) || defined(_M_X64)
    active_hal_pt = &x86_hal_pt_ops;
    active_hal_tlb = &x86_hal_tlb_ops;
#elif defined(__aarch64__) || defined(_M_ARM64)
    active_hal_pt = &arm64_hal_pt_ops;
    active_hal_tlb = &arm64_hal_tlb_ops;
#elif defined(__arm__)
    active_hal_pt = &arm32_hal_pt_ops;
    active_hal_tlb = &arm32_hal_tlb_ops;
#elif defined(__riscv) && __riscv_xlen == 64
    active_hal_pt = &riscv64_hal_pt_ops;
    active_hal_tlb = &riscv64_hal_tlb_ops;
#elif defined(__riscv) && __riscv_xlen == 32
    active_hal_pt = &riscv32_hal_pt_ops;
    active_hal_tlb = &riscv32_hal_tlb_ops;
#endif

#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(PROFILE_MPU_ONLY)
    // Overrides arch-specific setups for bare-metal MPU configs
    active_hal_pt = &mpu_hal_pt_ops;
    active_hal_tlb = &mpu_hal_tlb_ops;
#endif
}

void hal_tlb_init(void) {
    // Already set in hal_pt_init for simplicity
}

phys_addr_t hal_pt_create_address_space(phys_addr_t kernel_root_table) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || !active_hal_pt->create_address_space) {
        return 0U;
    }
    return active_hal_pt->create_address_space(kernel_root_table);
}

void hal_pt_destroy_address_space(phys_addr_t root_pt) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || !active_hal_pt->destroy_address_space) {
        return;
    }
    active_hal_pt->destroy_address_space(root_pt);
}

int hal_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || size == 0U || !is_page_aligned_u64(vaddr) || !is_page_aligned_u64(paddr) || !is_page_aligned_u64(size)) {
        return -1;
    }

    if (!active_hal_pt->map_range) {
        return -1;
    }
    return active_hal_pt->map_range(root_pt, vaddr, paddr, size, flags);
}

int hal_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || size == 0U || !is_page_aligned_u64(vaddr) || !is_page_aligned_u64(size)) {
        return -1;
    }

    if (!active_hal_pt->unmap_range) {
        return -1;
    }
    return active_hal_pt->unmap_range(root_pt, vaddr, size);
}

int hal_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || size == 0U || !is_page_aligned_u64(vaddr) || !is_page_aligned_u64(size)) {
        return -1;
    }

    if (!active_hal_pt->protect_range) {
        return -1;
    }
    return active_hal_pt->protect_range(root_pt, vaddr, size, new_flags);
}

int hal_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt) {
        return -1;
    }

    if (!active_hal_pt->query_mapping) {
        return -1;
    }
    return active_hal_pt->query_mapping(root_pt, vaddr, paddr, mapped_size, flags);
}
