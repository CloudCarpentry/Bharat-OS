#include "../../kernel/include/hal/hal_pt.h"
#include "../../kernel/include/hal/hal_pt_walk.h"
#include "../../kernel/include/hal/hal_tlb.h"

// Scaffold backend for ARM32 (MMU-Lite)
// This implements TRANSLATE_EXEC_MMU_LITE where the kernel has a small permanent
// window and full identity map is not possible.

static translate_backend_kind_t arm32_backend_type(void) { return TRANSLATE_BACKEND_MMU; }
static translate_exec_class_t arm32_exec_class(void) { return TRANSLATE_EXEC_MMU_LITE; }
static void* arm32_phys_to_virt(phys_addr_t phys) { (void)phys; return NULL; /* Needs dynamic kmap for non-linear */ }
static phys_addr_t arm32_virt_to_phys(const void* virt) { (void)virt; return 0; }
static bool arm32_has_linear_physmap(void) { return false; }
static phys_addr_t arm32_linear_physmap_base(void) { return 0; }
static phys_addr_t arm32_linear_physmap_limit(void) { return 0; }

static const hal_translate_ops_t arm32_translate_ops = {
    .backend_type = arm32_backend_type,
    .exec_class = arm32_exec_class,
    .phys_to_virt = arm32_phys_to_virt,
    .virt_to_phys = arm32_virt_to_phys,
    .has_linear_physmap = arm32_has_linear_physmap,
    .linear_physmap_base = arm32_linear_physmap_base,
    .linear_physmap_limit = arm32_linear_physmap_limit,
};

static phys_addr_t arm32_create_address_space(phys_addr_t kernel_root_table) {
    (void)kernel_root_table;
    return 0; // Stub
}

static void arm32_destroy_address_space(phys_addr_t root_pt) {
    (void)root_pt;
}

static int arm32_map_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)flags;
    return -1; // ENOSYS
}

static int arm32_unmap_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr) {
    (void)root_pt; (void)vaddr; (void)unmapped_paddr;
    return -1; // ENOSYS
}

static int arm32_protect_page(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags) {
    (void)root_pt; (void)vaddr; (void)new_flags;
    return -1; // ENOSYS
}

static int arm32_query_page(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags) {
    (void)root_pt; (void)vaddr; (void)paddr; (void)flags;
    return -1; // ENOSYS
}

static const hal_pt_caps_t arm32_pt_caps = {
    .backend_kind = TRANSLATE_BACKEND_MMU,
    .exec_class = TRANSLATE_EXEC_MMU_LITE,
    .supports_sparse_vm = false,
    .supports_demand_fault = false,
    .supports_protect = false,
    .supports_query = false,
    .supports_range_map = false,
    .supports_range_unmap = false,
    .supports_range_protect = false,
    .supports_asid = false,
    .supports_pcid = false,
    .supports_global = false,
    .supports_nx_or_xn = false,
    .supports_ad_bits = false,
    .supports_large_2m = false,
    .supports_large_1g = false,
    .supports_device_memtype = true,
    .supports_writecombine = false,
    .requires_bbm = false,
    .supports_cow_softbit = false,
    .supports_linear_physmap = false,
};

hal_pt_ops_t arm32_hal_pt_ops = {
    .backend_type          = TRANSLATE_BACKEND_MMU,
    .caps                  = &arm32_pt_caps,
    .create_address_space  = arm32_create_address_space,
    .destroy_address_space = arm32_destroy_address_space,
    .map_page              = arm32_map_page,
    .unmap_page            = arm32_unmap_page,
    .protect_page          = arm32_protect_page,
    .query_page            = arm32_query_page,
    .map_range             = NULL,
    .unmap_range           = NULL,
    .protect_range         = NULL,
    .query_mapping         = NULL,
};

static void arm32_tlb_flush_all_local(void) {
    // Stub
}

static const hal_tlb_caps_t arm32_tlb_caps = {
    .supports_page_flush = false,
    .supports_range_flush = false,
    .supports_aspace_flush = false,
    .supports_all_flush = true,
    .supports_remote_targeted_flush = false,
    .supports_broadcast_flush = false,
    .supports_asid_selective_flush = false,
    .supports_lazy_generation_model = false,
};

hal_tlb_ops_t arm32_hal_tlb_ops = {
    .caps                 = &arm32_tlb_caps,
    .flush_page_local      = NULL,
    .flush_range_local     = NULL,
    .flush_all_local       = arm32_tlb_flush_all_local,
    .flush_asid_local      = NULL,
    .flush_page_remote     = NULL,
    .flush_range_remote    = NULL,
    .flush_all_remote      = NULL,
    .flush_page_broadcast  = NULL,
    .flush_range_broadcast = NULL,
    .flush_all_broadcast   = NULL,
};

// Override symbol to avoid redefinition if built alongside others
#if defined(__arm__) && !defined(__aarch64__)
const hal_translate_ops_t* hal_translate_ops(void) {
    return &arm32_translate_ops;
}
#endif
