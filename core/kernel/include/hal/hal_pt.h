#ifndef BHARAT_HAL_PT_H
#define BHARAT_HAL_PT_H

#include <stdint.h>
#include <stddef.h>
#include "../../include/mm.h"

// Translation Backend Kind
typedef enum {
    TRANSLATE_BACKEND_NONE = 0,
    TRANSLATE_BACKEND_MMU,  // Traditional sparse page tables (x86_64, arm64, riscv64)
    TRANSLATE_BACKEND_MPU,  // Region isolation only (Cortex-M, EDGE32)
} translate_backend_kind_t;

// Translation Execution Class
typedef enum {
    TRANSLATE_EXEC_MMU_FULL,  // Linear physmap or high-half direct map
    TRANSLATE_EXEC_MMU_LITE,  // Permanent small kernel window or temporary mappings
    TRANSLATE_EXEC_MPU_ONLY,  // Identity mapping where valid
} translate_exec_class_t;

// Backend Translation Operations
typedef struct hal_translate_ops {
    translate_backend_kind_t (*backend_type)(void);
    translate_exec_class_t   (*exec_class)(void);

    void*       (*phys_to_virt)(phys_addr_t phys);
    phys_addr_t (*virt_to_phys)(const void* virt);

    bool        (*has_linear_physmap)(void);
    phys_addr_t (*linear_physmap_base)(void);   // optional
    phys_addr_t (*linear_physmap_limit)(void);  // optional
} hal_translate_ops_t;

const hal_translate_ops_t* hal_translate_ops(void);

// Common VM Permission Enum
typedef enum {
    VM_PERM_R   = (1 << 0),
    VM_PERM_W   = (1 << 1),
    VM_PERM_X   = (1 << 2),
    VM_PERM_U   = (1 << 3), // User accessible
} vm_perm_t;

// Common VM Memory Type / Cacheability Enum
typedef enum {
    VM_MEM_NORMAL,          // Standard cacheable RAM
    VM_MEM_DEVICE,          // Strictly ordered, uncacheable device memory
    VM_MEM_UNCACHED,        // Uncacheable memory
    VM_MEM_WRITE_COMBINE,   // Write-combining (e.g. for framebuffers)
    VM_MEM_DMA_COHERENT,    // Coherent DMA memory
    VM_MEM_DMA_STREAMING,   // Non-coherent DMA memory
} vm_memtype_t;

// Map Level/Granularity
typedef enum {
    MAP_LEVEL_4K,   // Base page
    MAP_LEVEL_2M,   // Huge page (x86_64 2M, ARM64 2M, RISCV 2M)
    MAP_LEVEL_1G,   // Gigantic page
    MAP_LEVEL_MPU,  // MPU region size (variable)
} map_level_t;

typedef struct hal_pt_caps {
    translate_backend_kind_t backend_kind;
    translate_exec_class_t exec_class;
    bool supports_sparse_vm;
    bool supports_demand_fault;
    bool supports_protect;
    bool supports_query;
    bool supports_range_map;
    bool supports_range_unmap;
    bool supports_range_protect;
    bool supports_asid;
    bool supports_pcid;
    bool supports_global;
    bool supports_nx_or_xn;
    bool supports_ad_bits;
    bool supports_large_2m;
    bool supports_large_1g;
    bool supports_device_memtype;
    bool supports_writecombine;
    bool requires_bbm;
    bool supports_cow_softbit;
    bool supports_linear_physmap;
} hal_pt_caps_t;

// Generic Page Table Flags (Legacy flags, preserved for now during migration)
#define HAL_PT_FLAG_READ    (1 << 0)
#define HAL_PT_FLAG_WRITE   (1 << 1)
#define HAL_PT_FLAG_EXEC    (1 << 2)
#define HAL_PT_FLAG_USER    (1 << 3)
#define HAL_PT_FLAG_GLOBAL  (1 << 4)
#define HAL_PT_FLAG_NOCACHE (1 << 5)
#define HAL_PT_FLAG_DEVICE  (1 << 6)
#define HAL_PT_FLAG_COW     (1 << 7)
#define HAL_PT_FLAG_LARGE_2M (1 << 8)
#define HAL_PT_FLAG_LARGE_1G (1 << 9)

// Architecture-neutral Page Table / Translation Backend API
typedef struct hal_pt_ops {
    translate_backend_kind_t backend_type;
    const hal_pt_caps_t *caps;

    // Lifecycle
    phys_addr_t (*create_address_space)(phys_addr_t kernel_root_table);
    void        (*destroy_address_space)(phys_addr_t root_pt);

    // Mapping
    int         (*map_page)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);
    int         (*unmap_page)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *unmapped_paddr);
    int         (*protect_page)(phys_addr_t root_pt, virt_addr_t vaddr, uint32_t new_flags);

    // Query
    int         (*query_page)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, uint32_t *flags);

    // Optional range/advanced operations. If NULL, generic wrappers are used.
    int         (*map_range)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags);
    int         (*unmap_range)(phys_addr_t root_pt, virt_addr_t vaddr, size_t size);
    int         (*protect_range)(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags);
    int         (*query_mapping)(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags);
} hal_pt_ops_t;

extern hal_pt_ops_t *active_hal_pt;

void hal_pt_init(void);

phys_addr_t hal_pt_create_address_space(phys_addr_t kernel_root_table);
void hal_pt_destroy_address_space(phys_addr_t root_pt);

int hal_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags);
int hal_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size);
int hal_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags);
int hal_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags);
const hal_pt_caps_t *hal_pt_caps(void);

#endif // BHARAT_HAL_PT_H
