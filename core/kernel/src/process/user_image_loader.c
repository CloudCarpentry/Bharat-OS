#include "process/user_image_loader.h"
#include "bharat/elf/elf_parser.h"
#include "mm.h"
#include "mm/physmap.h"
#include "mm/vm_mapping.h"
#include "lib/base/string.h"
#include "console/console_core.h"
#include "hal/hal.h"

// Temporarily undefine __KERNEL__ so we can include the UAPI header
// The build system adds both __KERNEL__ and __USER__ when compiling this object for some reason (or just __KERNEL__)
#ifdef __KERNEL__
#define __KERNEL_WAS_DEFINED__
#undef __KERNEL__
#endif

#ifdef __USER__
#define __USER_WAS_DEFINED__
#undef __USER__
#endif

#include <bharat/uapi/init/bootstrap.h>

#ifdef __KERNEL_WAS_DEFINED__
#define __KERNEL__ 1
#endif

#define BH_USER_STACK_DEFAULT_SIZE (64U * 1024U)

static void mem_zero(void *ptr, size_t size) {
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < size; ++i) {
        p[i] = 0;
    }
}

static void mem_copy(void *dst, const void *src, size_t size) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < size; ++i) {
        d[i] = s[i];
    }
}

kstatus_t bh_user_image_load(
    bh_process_t *process,
    address_space_t *aspace,
    const bh_user_image_t *image,
    bh_user_image_result_t *out)
{
    if (!process || !aspace || !image || !out) {
        return K_ERR_INVALID_ARG;
    }

    elf_summary_t summary;
    elf_parse_status_t p_status = elf_parse_image((const uint8_t *)image->bytes, image->size, &summary);
    if (p_status != ELF_PARSE_OK) {
        console_write_raw("[LOADER] ELF parse failed\n", 26);
        return K_ERR_INVALID_ARG;
    }

    console_write_raw("[BOOTSTRAP] ELF validated\n", 26);

    size_t load_count = 0;
    p_status = elf_get_load_segment_count((const uint8_t *)image->bytes, image->size, &load_count);
    if (p_status != ELF_PARSE_OK || load_count == 0) {
        return K_ERR_INVALID_ARG;
    }

    // Allocate an array for segments
    elf_segment_t segments[16];
    if (load_count > 16) {
        return K_ERR_NO_MEMORY;
    }

    size_t extracted = 0;
    p_status = elf_extract_load_segments((const uint8_t *)image->bytes, image->size, segments, 16, &extracted);
    if (p_status != ELF_PARSE_OK || extracted != load_count) {
        return K_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < extracted; ++i) {
        elf_segment_t *seg = &segments[i];

        uint32_t prot = VM_PROT_USER;
        if (seg->flags & 4) prot |= VM_PROT_READ; // PF_R
        if (seg->flags & 2) prot |= VM_PROT_WRITE; // PF_W
        if (seg->flags & 1) prot |= VM_PROT_EXEC; // PF_X

        // W^X Enforcement
        if ((prot & VM_PROT_WRITE) && (prot & VM_PROT_EXEC)) {
            console_write_raw("[LOADER] Rejecting W^X segment\n", 31);
            return K_ERR_DENIED;
        }

        uint64_t start_addr = seg->virtual_address;
        uint64_t aligned_start = start_addr & ~(PAGE_SIZE - 1);
        uint64_t offset_in_page = start_addr - aligned_start;
        uint64_t end_addr = start_addr + seg->memory_size;
        uint64_t aligned_end = (end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        uint64_t map_size = aligned_end - aligned_start;

        if (aligned_start < aspace->user_base || aligned_end > aspace->user_limit) {
            return K_ERR_INVALID_ARG;
        }

        uint32_t map_flags = VM_MAP_FIXED;
        if (prot & VM_PROT_EXEC) {
            map_flags |= VM_MAP_EXEC_OK;
        }

        vm_region_t *region;
        kstatus_t kst = aspace_region_reserve(aspace, aligned_start, map_size, prot, map_flags, VM_INHERIT_NONE, &region);
        if (kst != K_OK) return kst;

        // Allocate anonymous physical memory and map it (using naive individual page mapping for now)
        // Alternatively, we could create an anonymous vm_object, attach it, and let page faults handle it.
        // For early bootstrap, eager mapping is often safer.
        for (uint64_t off = 0; off < map_size; off += PAGE_SIZE) {
            void *page_ptr = pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO);
            if (!page_ptr) return K_ERR_NO_MEMORY;

            // Map into address space (bypassing full object layer for this raw bootstrap load)
            // Note: A more complete implementation should use aspace_map_region or prot_domain map.
            // But we can just use the prot_domain directly.
            prot_domain_map_region(aspace->prot_domain, aligned_start + off, (phys_addr_t)(uintptr_t)page_ptr, PAGE_SIZE, prot);
        }

        // Copy file data
        void *dst = (void *)(uintptr_t)(aligned_start); // In a high-half kernel, this user VA is not directly accessible!
        // We must map it or use physmap.
        for (uint64_t off = 0; off < map_size; off += PAGE_SIZE) {
             phys_addr_t paddr = 0;
             uint32_t out_prot = 0;
             prot_domain_query_region(aspace->prot_domain, aligned_start + off, &paddr, &out_prot);
             if (paddr) {
                 void *kvirt = physmap_phys_to_virt(paddr);
                 if (kvirt) {
                     uint64_t page_va_start = aligned_start + off;
                     uint64_t page_va_end = page_va_start + PAGE_SIZE;

                     uint64_t copy_start = (start_addr > page_va_start) ? start_addr : page_va_start;
                     uint64_t copy_end = (start_addr + seg->file_size < page_va_end) ? (start_addr + seg->file_size) : page_va_end;

                     if (copy_start < copy_end) {
                         size_t copy_len = copy_end - copy_start;
                         size_t file_offset = copy_start - start_addr;
                         mem_copy((uint8_t *)kvirt + (copy_start - page_va_start), (const uint8_t *)image->bytes + seg->file_offset + file_offset, copy_len);
                     }
                 }
             }
        }

        // BSS is already zeroed by PMM_ALLOC_ZERO
        console_write_raw("[BOOTSTRAP] PT_LOAD mapped\n", 27);
    }

    // 2. Allocate Stack
    uintptr_t stack_top = (aspace->user_limit & ~(PAGE_SIZE - 1)); // e.g. 0x00007FFFFFFFF000
    uintptr_t stack_base = stack_top - BH_USER_STACK_DEFAULT_SIZE;
    uintptr_t guard_base = stack_base - PAGE_SIZE;

    vm_region_t *stack_region;
    kstatus_t kst = aspace_region_reserve(aspace, stack_base, BH_USER_STACK_DEFAULT_SIZE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER, VM_MAP_FIXED, VM_INHERIT_NONE, &stack_region);
    if (kst != K_OK) return kst;

    for (uint64_t off = 0; off < BH_USER_STACK_DEFAULT_SIZE; off += PAGE_SIZE) {
        void *page_ptr = pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO);
        if (!page_ptr) return K_ERR_NO_MEMORY;
        prot_domain_map_region(aspace->prot_domain, stack_base + off, (phys_addr_t)(uintptr_t)page_ptr, PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER);
    }
    console_write_raw("[BOOTSTRAP] user stack created\n", 31);

    // Guard page (unmapped) is implicitly created by skipping allocation at guard_base

    // 3. Allocate and populate Startup Info page
    uintptr_t startup_va = guard_base - PAGE_SIZE;

    vm_region_t *startup_region;
    kst = aspace_region_reserve(aspace, startup_va, PAGE_SIZE, VM_PROT_READ | VM_PROT_USER, VM_MAP_FIXED, VM_INHERIT_NONE, &startup_region);
    if (kst != K_OK) return kst;

    void *startup_phys = pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO);
    if (!startup_phys) return K_ERR_NO_MEMORY;

    void *startup_kvirt = physmap_phys_to_virt((phys_addr_t)(uintptr_t)startup_phys);
    bharat_user_startup_t *startup = (bharat_user_startup_t *)startup_kvirt;

    startup->abi_version = 1;
    startup->struct_size = sizeof(bharat_user_startup_t);
    startup->argc = 0;
    startup->flags = 0;
    startup->argv = 0;
    startup->envp = 0;

    // Hardcode some minimal multikernel info for now, as defined
    startup->bootstrap.abi_version = 1;
    startup->bootstrap.struct_size = sizeof(bharat_bootstrap_info_t);
    startup->bootstrap.boot_session_id = 0x12345678; // A mock session ID
    startup->bootstrap.kernel_instance_id = 0;
    startup->bootstrap.home_core_id = hal_cpu_get_id();
    startup->bootstrap.available_kernel_mask = (1ULL << 0);
    startup->bootstrap.online_core_mask = (1ULL << hal_cpu_get_id());
    // Caps will be populated later or seeded.
    startup->bootstrap.self_process_cap = 0;
    startup->bootstrap.bootstrap_cap = 0;

    prot_domain_map_region(aspace->prot_domain, startup_va, (phys_addr_t)(uintptr_t)startup_phys, PAGE_SIZE, VM_PROT_READ | VM_PROT_USER);
    console_write_raw("[BOOTSTRAP] bootstrap capabilities installed\n", 45);

    out->entry_point = summary.entry_point;
    out->user_stack_top = stack_top;
    out->startup_va = startup_va;
    out->aspace = aspace;

    return K_OK;
}
