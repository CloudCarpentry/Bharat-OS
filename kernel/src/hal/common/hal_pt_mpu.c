#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_pt_walk.h"
#include "../../../include/hal/hal_tlb.h"

// Scaffold backend for MPU-Only Target (Cortex-M)
// Implements TRANSLATE_EXEC_MPU_ONLY with region packing rather than sparse page tables.

static translate_backend_kind_t mpu_backend_type(void) { return TRANSLATE_BACKEND_MPU; }
static translate_exec_class_t mpu_exec_class(void) { return TRANSLATE_EXEC_MPU_ONLY; }
static void* mpu_phys_to_virt(phys_addr_t phys) { return (void*)(uintptr_t)phys; /* 1:1 map */ }
static phys_addr_t mpu_virt_to_phys(const void* virt) { return (phys_addr_t)(uintptr_t)virt; }
static bool mpu_has_linear_physmap(void) { return true; }
static phys_addr_t mpu_linear_physmap_base(void) { return 0; }
static phys_addr_t mpu_linear_physmap_limit(void) { return ~0ULL; }

static const hal_translate_ops_t mpu_translate_ops = {
    .backend_type = mpu_backend_type,
    .exec_class = mpu_exec_class,
    .phys_to_virt = mpu_phys_to_virt,
    .virt_to_phys = mpu_virt_to_phys,
    .has_linear_physmap = mpu_has_linear_physmap,
    .linear_physmap_base = mpu_linear_physmap_base,
    .linear_physmap_limit = mpu_linear_physmap_limit,
};

// Instead of page tables, the "address space" ID is just a tracking token.
static phys_addr_t mpu_create_address_space(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    static uint32_t token = 1;
    return token++;
}

static void mpu_destroy_address_space(phys_addr_t root_pt) {
    (void)root_pt;
}

// In MPU systems, we do not map 4k pages dynamically. Memory mapping implies adding an MPU region rule.
static int mpu_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)flags;
    return -1; // ENOSYS
}

static int mpu_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    (void)root_pt; (void)vaddr; (void)unmapped_paddr;
    return -1; // ENOSYS
}

static int mpu_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    (void)root_pt; (void)vaddr; (void)new_flags;
    return -1; // ENOSYS
}

static int mpu_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)flags;
    return -1; // ENOSYS
}

// Region mapping logic (Region packer logic would interface here eventually)
// We would track these and program MPU slots on context switch.
static int mpu_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)size; (void)flags;
    return 0; // Return OK because in MPU it's 1:1, but record for protection boundary later
}

hal_pt_ops_t mpu_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MPU,
    .create_address_space  = mpu_create_address_space,
    .destroy_address_space = mpu_destroy_address_space,
    .map_page              = mpu_map_page,
    .unmap_page            = mpu_unmap_page,
    .protect_page          = mpu_protect_page,
    .query_page            = mpu_query_page,
    .map_range             = mpu_map_range,
    .unmap_range           = NULL,
    .protect_range         = NULL,
    .query_mapping         = NULL,
};

static void mpu_tlb_flush_all_local(void) {
    // Re-program MPU registers if necessary based on active tracking
}

hal_tlb_ops_t mpu_hal_tlb_ops = {
    .flush_page_local      = NULL,
    .flush_range_local     = NULL,
    .flush_all_local       = mpu_tlb_flush_all_local,
    .flush_asid_local      = NULL,
    .flush_page_remote     = NULL,
    .flush_range_remote    = NULL,
    .flush_all_remote      = NULL,
    .flush_page_broadcast  = NULL,
    .flush_range_broadcast = NULL,
    .flush_all_broadcast   = NULL,
};

// Provide translation ops if compiled explicitly for MPU profile
#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(PROFILE_MPU_ONLY)
const hal_translate_ops_t* hal_translate_ops(void) {
    return &mpu_translate_ops;
}
#endif
