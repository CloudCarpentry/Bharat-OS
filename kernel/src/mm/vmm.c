#include "mm.h"
#include "mm/aspace.h"
#include "hal/hal_mpa.h"
#include "hal/hal_tlb.h"
#include "hal/hal_pt.h"
#include "mm/tlb.h"
#include "capability.h"
#include "mm/pmm.h"
#include "slab.h"
#include <stddef.h>
#include <stdint.h>
#include "mm/aspace_profile.h"

address_space_t kernel_space;
static int kernel_space_ready = 0;
static phys_addr_t kernel_root_pt = 0;
static phys_addr_t bootstrap_root_pt = 0;  // Store original bootstrap CR3
static volatile int kernel_space_init_in_progress = 0;

static void clear_address_space(address_space_t *as) {
    volatile uint8_t *dst = (volatile uint8_t *)as;
    for (size_t i = 0; i < sizeof(address_space_t); i++) {
        dst[i] = 0U;
    }
}

static void init_kernel_space_from_bootstrap_root(phys_addr_t root_pt) {
    if (root_pt == 0U) {
        /*
         * Some architectures enter C with MMU/paging disabled (TTBR/SATP/CR3 root is 0).
         * In that case, bootstrap a fresh kernel root and keep root switch deferred
         * until architecture-specific mappings are guaranteed safe.
         */
        root_pt = hal_pt_create_address_space(0U);
    }

    clear_address_space(&kernel_space);
    kernel_space.root_pt = root_pt;
    kernel_space.user_base = 0x1000U;
    kernel_space.user_limit = 0x00007FFFFFFFFFFFULL;
    spin_lock_init(&kernel_space.lock);
    kernel_root_pt = root_pt;
    kernel_space_ready = (root_pt != 0U) ? 1 : 0;
}

static void ensure_kernel_space_ready(void) {
    if (kernel_space_ready) return;
    if (kernel_space_init_in_progress) return;

    kernel_space_init_in_progress = 1;

    /*
     * During early VMM initialization we prefer adopting the active hardware
     * root via get_root(). If the architecture boots in MMU-off/bare mode and
     * get_root() returns 0, we bootstrap a fresh root in
     * init_kernel_space_from_bootstrap_root().
     */
    if (active_mem_protect && active_mem_protect->cpu_ops.get_root) {
        phys_addr_t bootstrap_root = active_mem_protect->cpu_ops.get_root();
        init_kernel_space_from_bootstrap_root(bootstrap_root);
    } else {
        init_kernel_space_from_bootstrap_root(0U);
    }

    kernel_space_init_in_progress = 0;
}

#include "mm/prot_domain.h"

int vmm_init(void) {
    // CRITICAL: Capture bootstrap CR3 before any CR3 switches
    // This is the hardware root with complete identity + high-half mappings
    if (!bootstrap_root_pt && active_mem_protect && active_mem_protect->cpu_ops.get_root) {
        bootstrap_root_pt = active_mem_protect->cpu_ops.get_root();
    }
    
    prot_domain_init();
    ensure_kernel_space_ready();

    /*
     * Early boot-safe root handoff policy:
     * - If we can read the current root and it is non-zero and different from
     *   the freshly created kernel root, keep the bootstrap root active.
     * - Otherwise (same root or unknown/zero current root), perform set_root.
     *
     * This avoids unsafe early CR3/SATP switches that can drop bootstrap
     * mappings, while still allowing idempotent/required activation paths.
     * 
     * SPECIAL CASE for RISC-V64: When starting in bare mode (SATP=0), the new
     * page table has no kernel mappings. Skip set_root() and continue using
     * bare mode until proper page tables are set up later.
     */
    if (kernel_space_ready && kernel_space.root_pt != 0U &&
        active_mem_protect && active_mem_protect->cpu_ops.set_root) {
        phys_addr_t current_root = 0U;
        if (active_mem_protect->cpu_ops.get_root) {
            current_root = active_mem_protect->cpu_ops.get_root();
        }

        // RISC-V bare mode can start with SATP=0 and an empty newly created root.
        // Keep the previous behavior there; other architectures (e.g. arm64)
        // should activate the prepared root so MMU and control registers are set.
#if defined(__riscv)
        if (current_root == 0U && bootstrap_root_pt == 0U) {
            // Skip set_root in bare mode without bootstrap on RISC-V
        } else if (current_root == 0U || current_root == kernel_space.root_pt) {
            active_mem_protect->cpu_ops.set_root(kernel_space.root_pt);
        }
#else
        if (current_root == 0U || current_root == kernel_space.root_pt) {
            active_mem_protect->cpu_ops.set_root(kernel_space.root_pt);
        }
#endif
    }

    return kernel_space_ready ? 0 : -1;
}

phys_addr_t vmm_get_kernel_root(void) {
    // Multi-kernel architecture: Always use bootstrap root as authoritative kernel root
    // Bootstrap page tables have complete low identity + high canonical mappings
    // required for per-core kernel instances with separate page tables
    if (bootstrap_root_pt != 0) {
        return bootstrap_root_pt;
    }
    
    // Fallback: try to read current root (early boot path)
    if (active_mem_protect && active_mem_protect->cpu_ops.get_root) {
        phys_addr_t current_root = active_mem_protect->cpu_ops.get_root();
        if (current_root != 0) {
            // Cache it for future calls
            bootstrap_root_pt = current_root;
            return current_root;
        }
    }
    
    if (kernel_root_pt != 0) return kernel_root_pt;
    return 0;
}

int vmm_is_kernel_space_ready(void) {
    return kernel_space_ready;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as) return -1;

    // Use the unified MPA HAL abstraction instead of the old MMU ops or prot_domain
    if (!active_mem_protect || !active_mem_protect->cpu_ops.map_page) return -1;

    // Look up authoritative region (kept for legacy/bookkeeping compatibility)
    vm_region_t *region = aspace_lookup_region(as, vaddr);
    (void)region;

    // Translate VMM flags to MPA Capability Bits
    uint32_t mpa_flags = 0;
    if (flags & HAL_PT_FLAG_WRITE) mpa_flags |= MPA_CAP_WRITE;
    if (flags & HAL_PT_FLAG_EXEC) mpa_flags |= MPA_CAP_EXEC_PERM;
    if (flags & PAGE_USER) mpa_flags |= MPA_CAP_USER;
    if (flags & (0x40 | 0x80)) mpa_flags |= MPA_CAP_DEVICE; // Old GPU and NPU masks

    // Use the HAL abstraction
    int ret = active_mem_protect->cpu_ops.map_page(as->root_pt, vaddr, paddr, mpa_flags);

    if (ret == 0 && active_mem_protect->cpu_ops.flush_tlb_local) {
        active_mem_protect->cpu_ops.flush_tlb_local(vaddr, 0); // ASID 0 for now
    }
    return ret;
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as) return -1;

    if (!active_mem_protect || !active_mem_protect->cpu_ops.unmap_page) return -1;

    phys_addr_t unmapped_pa;
    int ret = active_mem_protect->cpu_ops.unmap_page(as->root_pt, vaddr, &unmapped_pa);

    if (ret == 0 && active_mem_protect->cpu_ops.flush_tlb_local) {
        active_mem_protect->cpu_ops.flush_tlb_local(vaddr, 0); // ASID 0 for now
    }
    return ret;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    ensure_kernel_space_ready();
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    ensure_kernel_space_ready();
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

#include "mm/fault.h"
#include "mm/vm_mapping.h"

int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = vaddr,
        .access = VM_PROT_WRITE,
        .arch_code = 0
    };
    vm_fault_result_t res = vm_handle_fault(&event);
    return (res == VM_FAULT_RESOLVED) ? 0 : -1;
}

address_space_t *mm_create_address_space(void) {
    uint32_t create_flags = 0; // Legacy basic creation
    address_space_t *as = NULL;

    // Defer explicit legality checks entirely to the authoritative aspace_create boundary
    if (aspace_create(&as, create_flags) != 0) return NULL;
    return as;
}

void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
    extern void vmm_process_urpc_messages(void);
    vmm_process_urpc_messages();
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t *cap, int is_npu) {
    (void)is_npu;
    if (!cap) return -1;
    return vmm_map_page(vaddr, paddr, cap->rights_mask);
}
