#include "../../include/hal/vmm.h"
#include "../../include/hal/mmu_ops.h"
#include "../../include/numa.h"

// -----------------------------------------------------------------------------
// mmu_ops_t Wrapper implementations for ARM32 Cortex-M (MPU only)
// -----------------------------------------------------------------------------

// With an MPU, we don't really "create tables". MPU regions are hardware registers.
// However, to satisfy the API, we can return a dummy physical address.
static phys_addr_t arm32_mpu_create_table(void) {
    return 0x1000; // Dummy value indicating "initialized"
}

static void arm32_mpu_destroy_table(phys_addr_t root) {
    (void)root; // No-op
}

static phys_addr_t arm32_mpu_clone_kernel(phys_addr_t kernel_root) {
    (void)kernel_root;
    return 0x1000; // All processes share the MPU regions
}

// Map a region using MPU logic. In a real system, this configures RBAR and RASR.
static int arm32_mpu_map(phys_addr_t root, virt_addr_t virt, phys_addr_t phys,
                         size_t size, mmu_flags_t flags) {
    (void)root;
    (void)virt;
    (void)phys;
    (void)size;
    (void)flags;

    // e.g., mpu_map_region(...)
    return 0;
}

static int arm32_mpu_unmap(phys_addr_t root, virt_addr_t virt, size_t size, phys_addr_t *unmapped_phys) {
    (void)root;
    (void)virt;
    (void)size;

    if (unmapped_phys) {
        *unmapped_phys = virt; // Identity mapping assumption on MCU
    }
    return 0;
}

static int arm32_mpu_protect(phys_addr_t root, virt_addr_t virt, size_t size, mmu_flags_t new_flags) {
    (void)root;
    (void)virt;
    (void)size;
    (void)new_flags;
    return 0;
}

static int arm32_mpu_query(phys_addr_t root, virt_addr_t virt, phys_addr_t *phys_out, mmu_flags_t *flags_out) {
    (void)root;
    if (phys_out) *phys_out = virt; // Identity mapping
    if (flags_out) *flags_out = MMU_READ | MMU_WRITE | MMU_EXEC;
    return 0;
}

static void arm32_mpu_activate(phys_addr_t root) {
    (void)root;
    // e.g., Enable MPU
}

static void arm32_mpu_deactivate(void) {
    // e.g., Disable MPU
}

static void arm32_mpu_tlb_flush_page(virt_addr_t virt) {
    (void)virt;
    // No-op for MPU
}

static void arm32_mpu_tlb_flush_all(void) {
    // No-op for MPU
}

static void arm32_mpu_tlb_flush_asid(uint16_t asid) {
    (void)asid;
}

mmu_ops_t arm32_mpu_ops = {
    .create_table     = arm32_mpu_create_table,
    .destroy_table    = arm32_mpu_destroy_table,
    .clone_kernel     = arm32_mpu_clone_kernel,
    .map              = arm32_mpu_map,
    .unmap            = arm32_mpu_unmap,
    .protect          = arm32_mpu_protect,
    .query            = arm32_mpu_query,
    .activate         = arm32_mpu_activate,
    .deactivate       = arm32_mpu_deactivate,
    .tlb_flush_page   = arm32_mpu_tlb_flush_page,
    .tlb_flush_all    = arm32_mpu_tlb_flush_all,
    .tlb_flush_asid   = arm32_mpu_tlb_flush_asid,

    .page_size        = 32, // MPU regions can be as small as 32 bytes on some Cortex-M
    .huge_page_sizes  = NULL,
    .levels           = 1, // Flat
    .has_nx           = true,
    .asid_bits        = 0,
    .has_user_kernel_split = false,
};

void arm32_mmu_detect(mmu_ops_t *ops) {
    (void)ops;
}
