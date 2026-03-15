#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/hal/hal.h"
#include "../../include/hal/vmm.h"
#include "../../include/hal/mmu_ops.h"
#include "../../include/advanced/formal_verif.h"
#include "../../include/sched.h"
#include "../../include/capability.h"
#include "../../include/mm_zswap.h"
#include "../../include/mm/address_token.h"
#include "../../include/security/isolation.h"

// @cite L4 Microkernels: The Lessons from 20 Years of Research and Implementation (Klein & Andronick, 2016)
// L4 minimal memory mapping model: Kernel only maps/unmaps physical pages, policy lives in user space.
#include <stddef.h>
#include <stdint.h>

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

static address_space_t kernel_space;

// Helper to translate old VMM flags to new MMU flags
static mmu_flags_t convert_vmm_flags(uint32_t flags) {
    mmu_flags_t mmu_flags = MMU_READ;
    if (flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
    if (flags & PAGE_USER)       mmu_flags |= MMU_USER;
    if (flags & PAGE_COW)        mmu_flags |= MMU_COW;
    return mmu_flags;
}

#include "../../include/profile.h"
#include "../../include/urpc/urpc_bootstrap.h"

#define URPC_MSG_TLB_SHOOTDOWN 0x01

void vmm_process_urpc_messages(void) {
    uint32_t current_core = hal_cpu_get_id();
    uint64_t msg;

    // Drain messages
    while (urpc_recv(current_core, &msg) == 0) {
        uint64_t type = msg & 0xFFF; // Extract type from bottom 12 bits
        virt_addr_t vaddr = (virt_addr_t)(msg & ~0xFFFULL); // Recover 4K aligned address

        if (type == URPC_MSG_TLB_SHOOTDOWN) {
            if (active_mmu && active_mmu->tlb_flush_page) {
                active_mmu->tlb_flush_page(vaddr);
            } else {
                hal_tlb_flush((unsigned long long)vaddr);
            }
        }
    }
}

int vmm_init(void) {
    arch_mmu_init();

    if (get_memory_model() == MEM_MODEL_MPU || get_memory_model() == MEM_MODEL_FLAT) {
        // MPU and FLAT memory models do not use traditional VMM page tables.
        // Return 0 indicating success, but VMM operations will be no-ops or handled
        // by specific MPU region managers not present in this generic VMM.
        return 0;
    }

    if (!active_mmu) {
        return -1; // Fallback or unsupported architecture
    }

    phys_addr_t root_dir_phys = active_mmu->create_table();
    if (root_dir_phys == 0U) {
        return -1;
    }

    kernel_space.root_table = root_dir_phys;
    return 0;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (get_memory_model() != MEM_MODEL_MMU) {
        return 0; // No-op for MPU/FLAT
    }

    if (!as || as->root_table == 0U || paddr == 0U) {
        return -1;
    }

    if (((vaddr & (PAGE_SIZE - 1U)) != 0U) || ((paddr & (PAGE_SIZE - 1U)) != 0U)) {
        return -1;
    }

    phys_addr_t existing_paddr = 0U;
    uint32_t existing_flags = 0U;
    if (hal_vmm_get_mapping(as->root_table, vaddr, &existing_paddr, &existing_flags) == 0) {
        (void)existing_flags;
        if (existing_paddr != paddr) {
            return -2;
        }
        return 0;
    }

    if ((flags & PAGE_COW) != 0U) {
        mm_inc_page_ref(paddr);
        flags &= ~CAP_RIGHT_WRITE;
    }

    mmu_flags_t mmu_flags = convert_vmm_flags(flags);
    return active_mmu->map(as->root_table, vaddr, paddr, PAGE_SIZE, mmu_flags);
}

// Global TLB shootdown function via URPC
void tlb_shootdown(virt_addr_t vaddr) {
    if (active_mmu && active_mmu->tlb_flush_page) {
        active_mmu->tlb_flush_page(vaddr);
    } else {
        hal_tlb_flush((unsigned long long)vaddr);
    }

    // URPC based shootdown
    uint32_t current_core = hal_cpu_get_id();
    // Pack message type into the bottom 12 bits (assuming vaddr is 4K aligned)
    uint64_t msg = ((uint64_t)vaddr & ~0xFFFULL) | (URPC_MSG_TLB_SHOOTDOWN & 0xFFF);

    for (uint32_t i = 0; i < 8; ++i) { // MAX_SUPPORTED_CORES is 8
        if (i != current_core && urpc_is_ready(i)) {
            // Attempt to send via URPC; if full, we could fall back to IPI
            // but the architecture calls for URPC as the primary vehicle.
            urpc_send(i, msg);
            // We should also trigger an IPI to notify the remote core that a message
            // is waiting in its queue, as URPC_send is just a queue primitive.
            hal_send_ipi_payload(i, 0);
        }
    }
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (get_memory_model() != MEM_MODEL_MMU) {
        return 0; // No-op for MPU/FLAT
    }

    if (!as || as->root_table == 0U) {
        return -1;
    }

    phys_addr_t paddr = 0;
    int ret = active_mmu->unmap(as->root_table, vaddr, PAGE_SIZE, &paddr);
    if (ret < 0) {
        return ret;
    }

    if (paddr != 0) {
        // Only reclaim to zswap if it isn't completely free, but here unmap means we might free it.
        // Assuming page reclaim daemon calls a function to explicitly compress. Let's just hook zswap in cow fault for uncompression.
        // If unmapping user page, try to reclaim it (for the sake of PoC).
        // Actually, normally the reclaim daemon does `zswap_store_page(paddr)`.
        mm_free_page(paddr);
    }

    tlb_shootdown(vaddr);
    return 0;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    if (!cap) {
        return -3; // ERR_CAP_DENIED / IPC_ERR_WOULD_BLOCK
    }

    uint32_t required_right = is_npu ? CAP_RIGHT_DEVICE_NPU : CAP_RIGHT_DEVICE_GPU;
    if ((cap->rights_mask & required_right) == 0U) {
        return -3; // ERR_CAP_DENIED
    }

    return mm_vmm_map_page(&kernel_space, vaddr, paddr, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
}

int vmm_map_device_mmio_token(virt_addr_t vaddr, phys_addr_t paddr, uint64_t size, const bharat_addr_token_t* token, int is_npu) {
    if (!token) {
        return -3; // ERR_CAP_DENIED
    }

    // Validate the token for Read/Write MMIO access
    int validation_res = bharat_addr_token_validate(token, paddr, size, ADDR_TOKEN_FLAG_READ | ADDR_TOKEN_FLAG_WRITE);
    if (validation_res < 0) {
        return -3; // ERR_CAP_DENIED
    }

    // Example checks, could expand class logic here
    if (token->iso_class == ISOLATION_CLASS_USER && !is_npu) {
       // Just as an example, user classes might not map GPU directly without extra capabilities
    }

    return mm_vmm_map_page(&kernel_space, vaddr, paddr, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
}

address_space_t* mm_create_address_space(void) {
    if (get_memory_model() != MEM_MODEL_MMU) {
        // For MPU/FLAT return a valid pointer, but no page tables are cloned.
        address_space_t* as = (address_space_t*)(uintptr_t)mm_alloc_page(NUMA_NODE_ANY);
        if (as) {
            as->root_table = 0;
            as->owner_core_id = hal_cpu_get_id();
            as->object_id = (uint64_t)(uintptr_t)as;
        }
        return as;
    }

    if (!active_mmu) return NULL;

    phys_addr_t root = active_mmu->clone_kernel(kernel_space.root_table);
    if (root == 0U) {
        return NULL;
    }

    address_space_t* as = (address_space_t*)(uintptr_t)mm_alloc_page(NUMA_NODE_ANY);
    if (!as) {
        mm_free_page(root);
        return NULL;
    }

    as->root_table = root;
    as->owner_core_id = hal_cpu_get_id(); // The core creating it initially owns it
    as->object_id = (uint64_t)(uintptr_t)as; // Simple object ID for now
    return as;
}

// Copy-on-Write Fault Handler
int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    if (get_memory_model() != MEM_MODEL_MMU) {
        return -1; // MPU/FLAT do not support demand paging / CoW
    }

    if (!as || as->root_table == 0U) return -1;

    phys_addr_t old_phys = 0;
    mmu_flags_t old_mmu_flags = 0;

    int ret = active_mmu->query(as->root_table, vaddr, &old_phys, &old_mmu_flags);
    if (ret < 0 || old_phys == 0) return -2;

    // Convert back if necessary or handle logic based on generic flags. For now we emulate the old behavior:
    uint32_t old_flags = 0;
    if (old_mmu_flags & MMU_WRITE) old_flags |= CAP_RIGHT_WRITE;
    if (old_mmu_flags & MMU_USER)  old_flags |= PAGE_USER;
    if (old_mmu_flags & MMU_COW)   old_flags |= PAGE_COW;

    // Allocate new page
    phys_addr_t new_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (!new_phys) return -1;

    // Copy data
    uint8_t* src = (uint8_t*)P2V(old_phys);
    uint8_t* dst = (uint8_t*)P2V(new_phys);
    for (int i = 0; i < PAGE_SIZE; i++) {
        dst[i] = src[i];
    }

    // Update mapping with Write permission, clear CoW
    uint32_t new_flags = (old_flags | CAP_RIGHT_WRITE) & ~PAGE_COW;
    mmu_flags_t new_mmu_flags = convert_vmm_flags(new_flags);

    ret = active_mmu->protect(as->root_table, vaddr, PAGE_SIZE, new_mmu_flags);
    if (ret < 0) {
        mm_free_page(new_phys);
        return ret;
    }

    // Decrement old page reference
    mm_free_page(old_phys);

    tlb_shootdown(vaddr);
    return 0;
}
