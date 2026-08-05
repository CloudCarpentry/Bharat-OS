#include "process/user_image_loader.h"
#include "bharat/elf/elf_parser.h"
#include "bharat/elf/elf_load_plan.h"
#include "mm.h"
#include "slab.h"
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
#define BH_LOADER_MAX_PAGES ((16U * 1024U * 1024U) / PAGE_SIZE)

typedef struct {
    uintptr_t va;
    void *page;
} loader_page_t;

typedef struct {
    address_space_t *aspace;
    uintptr_t regions[18];
    uint32_t region_count;
    loader_page_t pages[BH_LOADER_MAX_PAGES];
    uint32_t page_count;
} loader_txn_t;

static kstatus_t elf_plan_prot_to_vm(uint32_t plan_prot, uint32_t *out_vm_prot) {
    const uint32_t known = BH_ELF_PROT_READ | BH_ELF_PROT_WRITE | BH_ELF_PROT_EXEC | BH_ELF_PROT_USER;
    if (!out_vm_prot || (plan_prot & ~known) != 0U || (plan_prot & BH_ELF_PROT_USER) == 0U) {
        return K_ERR_INVALID_ARG;
    }
    if ((plan_prot & BH_ELF_PROT_WRITE) != 0U && (plan_prot & BH_ELF_PROT_EXEC) != 0U) {
        return K_ERR_DENIED;
    }
    uint32_t vm = 0;
    if ((plan_prot & BH_ELF_PROT_READ) != 0U) vm |= VM_PROT_READ;
    if ((plan_prot & BH_ELF_PROT_WRITE) != 0U) vm |= VM_PROT_WRITE;
    if ((plan_prot & BH_ELF_PROT_EXEC) != 0U) vm |= VM_PROT_EXEC;
    if ((plan_prot & BH_ELF_PROT_USER) != 0U) vm |= VM_PROT_USER;
    *out_vm_prot = vm;
    return K_OK;
}

static bh_elf_machine_t loader_expected_machine(bool *supported) {
    *supported = true;
#if defined(__x86_64__)
    return BH_ELF_MACHINE_X86_64;
#elif defined(__aarch64__)
    return BH_ELF_MACHINE_AARCH64;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return BH_ELF_MACHINE_RISCV64;
#else
    *supported = false;
    return BH_ELF_MACHINE_X86_64;
#endif
}

static void loader_txn_rollback(loader_txn_t *txn) {
    bool cleanup_failed = false;
    for (uint32_t i = txn->page_count; i > 0; --i) {
        loader_page_t *page = &txn->pages[i - 1];
        if (prot_domain_unmap_region(txn->aspace->prot_domain, page->va, PAGE_SIZE) != K_OK) {
            cleanup_failed = true;
        }
        pmm_free_page(page->page);
    }
    for (uint32_t i = txn->region_count; i > 0; --i) {
        if (aspace_region_detach(txn->aspace, txn->regions[i - 1]) != K_OK) {
            cleanup_failed = true;
        }
    }
    if (cleanup_failed) {
        console_write_raw("[LOADER] rollback incomplete; poisoning address space\n", 52);
        aspace_mark_poisoned(txn->aspace);
    }
}

static void mem_copy(void *dst, const void *src, size_t size) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < size; ++i) {
        d[i] = s[i];
    }
}


static void loader_write(const char *s) {
    console_write_raw(s, string_length(s));
}

static void loader_print_hex64(uint64_t value) {
    char buf[18];
    static const char hex[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (uint32_t i = 0; i < 16U; ++i) {
        uint32_t shift = (15U - i) * 4U;
        buf[2U + i] = hex[(value >> shift) & 0xFU];
    }
    console_write_raw(buf, sizeof(buf));
}

static void loader_print_u32_dec(uint32_t value) {
    char buf[10];
    uint32_t pos = sizeof(buf);
    do {
        buf[--pos] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && pos > 0U);
    console_write_raw(&buf[pos], sizeof(buf) - pos);
}

static void loader_print_kstatus(kstatus_t status) {
    if (status == K_OK) {
        loader_write("K_OK");
    } else if (status == K_ERR_INVALID_ARG) {
        loader_write("K_ERR_INVALID_ARG");
    } else if (status == K_ERR_UNSUPPORTED) {
        loader_write("K_ERR_UNSUPPORTED");
    } else if (status == K_ERR_DENIED) {
        loader_write("K_ERR_DENIED");
    } else if (status == K_ERR_NO_RESOURCES) {
        loader_write("K_ERR_NO_RESOURCES");
    } else if (status == K_ERR_NO_MEMORY) {
        loader_write("K_ERR_NO_MEMORY");
    } else if (status == K_ERR_VM_UNMAPPED) {
        loader_write("K_ERR_VM_UNMAPPED");
    } else {
        loader_write("K_ERR_OTHER");
    }
}

static const char *loader_machine_name(bh_elf_machine_t machine) {
    switch (machine) {
    case BH_ELF_MACHINE_X86_64:
        return "X86_64";
    case BH_ELF_MACHINE_AARCH64:
        return "AARCH64";
    case BH_ELF_MACHINE_RISCV64:
        return "RISCV64";
    default:
        return "UNKNOWN";
    }
}

static const char *loader_plan_status_name(int plan_res) {
    switch (plan_res) {
    case BH_ELF_PLAN_SUCCESS:
        return "BH_ELF_PLAN_SUCCESS";
    case BH_ELF_PLAN_ERR_MALFORMED:
        return "BH_ELF_PLAN_ERR_MALFORMED";
    case BH_ELF_PLAN_ERR_BOUNDS:
        return "BH_ELF_PLAN_ERR_BOUNDS";
    case BH_ELF_PLAN_ERR_OVERLAP:
        return "BH_ELF_PLAN_ERR_OVERLAP";
    case BH_ELF_PLAN_ERR_WX:
        return "BH_ELF_PLAN_ERR_WX";
    case BH_ELF_PLAN_ERR_ENTRY:
        return "BH_ELF_PLAN_ERR_ENTRY";
    case BH_ELF_PLAN_ERR_UNSUPPORTED:
        return "BH_ELF_PLAN_ERR_UNSUPPORTED";
    case BH_ELF_PLAN_ERR_LIMIT:
        return "BH_ELF_PLAN_ERR_LIMIT";
    case BH_ELF_PLAN_ERR_HEADER_SIZE:
        return "BH_ELF_PLAN_ERR_HEADER_SIZE";
    case BH_ELF_PLAN_ERR_MAGIC:
        return "BH_ELF_PLAN_ERR_MAGIC";
    case BH_ELF_PLAN_ERR_CLASS:
        return "BH_ELF_PLAN_ERR_CLASS";
    case BH_ELF_PLAN_ERR_TYPE:
        return "BH_ELF_PLAN_ERR_TYPE";
    case BH_ELF_PLAN_ERR_PARSE_SUMMARY:
        return "BH_ELF_PLAN_ERR_PARSE_SUMMARY";
    case BH_ELF_PLAN_ERR_SEGMENT_COUNT:
        return "BH_ELF_PLAN_ERR_SEGMENT_COUNT";
    case BH_ELF_PLAN_ERR_EXTRACT_SEGMENTS:
        return "BH_ELF_PLAN_ERR_EXTRACT_SEGMENTS";
    case BH_ELF_PLAN_ERR_FILE_MEM_SIZE:
        return "BH_ELF_PLAN_ERR_FILE_MEM_SIZE";
    case BH_ELF_PLAN_ERR_ALIGNMENT:
        return "BH_ELF_PLAN_ERR_ALIGNMENT";
    default:
        return "BH_ELF_PLAN_ERR_UNKNOWN";
    }
}

static void loader_print_fail(const char *stage, kstatus_t status) {
    loader_write("INIT_LOAD_FAIL: stage=");
    console_write_raw(stage, string_length(stage));
    loader_write(" status=");
    loader_print_kstatus(status);
    loader_write("\nBOOT_FAIL: component=INIT stage=");
    console_write_raw(stage, string_length(stage));
    loader_write(" status=");
    loader_print_kstatus(status);
    loader_write("\n");
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

    *out = (bh_user_image_result_t){0};

    bool machine_supported = false;
    bh_elf_machine_t expected_machine = loader_expected_machine(&machine_supported);
    if (!machine_supported) {
        loader_print_fail("ELF_HEADER", K_ERR_UNSUPPORTED);
        return K_ERR_UNSUPPORTED;
    }

    const uint8_t *b = (const uint8_t *)image->bytes;
    loader_write("INIT_LOAD_BEGIN: source=BOOT_MODULE phys_start=");
    loader_print_hex64((uint64_t)(uintptr_t)image->bytes);
    loader_write(" size=");
    loader_print_hex64((uint64_t)image->size);
    loader_write(" magic=");
    uint32_t magic = 0U;
    if (image->size >= 4U && b) {
        magic = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
                ((uint32_t)b[2] << 8) | (uint32_t)b[3];
    }
    loader_print_hex64(magic);
    loader_write(" elf_class=");
    loader_print_u32_dec((image->size > 4U && b) ? b[4] : 0U);
    loader_write(" expected_machine=");
    const char *machine_name = loader_machine_name(expected_machine);
    console_write_raw(machine_name, string_length(machine_name));
    loader_write("\n");

    bh_user_image_plan_v1_t plan;
    int plan_res = bh_elf_generate_load_plan_for_machine((const uint8_t *)image->bytes, image->size, aspace->user_base, aspace->user_limit, expected_machine, &plan);
    if (plan_res != BH_ELF_PLAN_SUCCESS) {
        kstatus_t mapped = plan_res == BH_ELF_PLAN_ERR_UNSUPPORTED ? K_ERR_UNSUPPORTED : K_ERR_INVALID_ARG;
        loader_write("INIT_LOAD_FAIL: stage=ELF_PLAN status=");
        const char *plan_status = loader_plan_status_name(plan_res);
        console_write_raw(plan_status, string_length(plan_status));
        loader_write("\nBOOT_FAIL: component=INIT stage=ELF_PLAN status=");
        console_write_raw(plan_status, string_length(plan_status));
        loader_write("\n");
        return mapped;
    }
    loader_write("INIT_STAGE: ELF_HEADER_VALID\n");
    loader_write("INIT_STAGE: ELF_PLAN_VALID\n");

    loader_txn_t *txn = (loader_txn_t *)kmalloc(sizeof(loader_txn_t));
    if (!txn) return K_ERR_NO_MEMORY;
    memset(txn, 0, sizeof(*txn));
    txn->aspace = aspace;
    kstatus_t status = K_OK;

    for (uint32_t i = 0; i < plan.segment_count; ++i) {
        bh_elf_load_segment_v1_t *seg = &plan.segments[i];
        uint32_t prot = 0;
        status = elf_plan_prot_to_vm(seg->prot, &prot);
        if (status != K_OK) { loader_print_fail("ELF_PROTECTION", status); goto fail; }

        uint64_t start_addr = seg->virtual_address;
        uint64_t aligned_start = start_addr & ~(PAGE_SIZE - 1ULL);
        uint64_t end_addr = start_addr + seg->memory_size;
        uint64_t aligned_end = (end_addr + PAGE_SIZE - 1ULL) & ~(PAGE_SIZE - 1ULL);
        uint64_t map_size = aligned_end - aligned_start;

        uint32_t map_flags = VM_MAP_FIXED;
        if ((prot & VM_PROT_EXEC) != 0U) map_flags |= VM_MAP_EXEC_OK;

        vm_region_t *region;
        status = aspace_region_reserve(aspace, aligned_start, map_size, prot, map_flags, VM_INHERIT_NONE, &region);
        if (status != K_OK) { loader_print_fail("REGION_RESERVE", status); goto fail; }
        txn->regions[txn->region_count++] = (uintptr_t)aligned_start;

        for (uint64_t off = 0; off < map_size; off += PAGE_SIZE) {
            if (txn->page_count >= BH_LOADER_MAX_PAGES) { status = K_ERR_NO_RESOURCES; loader_print_fail("PAGE_MAP", status); goto fail; }
            void *page_ptr = pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO);
            if (!page_ptr) { status = K_ERR_NO_MEMORY; loader_print_fail("PAGE_MAP", status); goto fail; }
            status = prot_domain_map_region(aspace->prot_domain, aligned_start + off, (phys_addr_t)(uintptr_t)page_ptr, PAGE_SIZE, prot);
            if (status != K_OK) { pmm_free_page(page_ptr); loader_print_fail("PAGE_MAP", status); goto fail; }
            txn->pages[txn->page_count++] = (loader_page_t){.va = (uintptr_t)(aligned_start + off), .page = page_ptr};
        }

        for (uint64_t off = 0; off < map_size; off += PAGE_SIZE) {
            uintptr_t paddr = 0;
            uint32_t out_prot = 0;
            status = prot_domain_query_region(aspace->prot_domain, aligned_start + off, &paddr, &out_prot);
            if (status != K_OK || paddr == 0) { status = status != K_OK ? status : K_ERR_VM_UNMAPPED; loader_print_fail("MODULE_MAPPED", status); goto fail; }
            void *kvirt = physmap_phys_to_virt(paddr);
            if (!kvirt) { status = K_ERR_VM_UNMAPPED; loader_print_fail("MODULE_MAPPED", status); goto fail; }
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
    loader_write("INIT_STAGE: SEGMENTS_MAPPED\n");

    uintptr_t stack_top = (aspace->user_limit & ~(PAGE_SIZE - 1ULL));
    uintptr_t stack_base = stack_top - BH_USER_STACK_DEFAULT_SIZE;
    uintptr_t guard_base = stack_base - PAGE_SIZE;
    vm_region_t *stack_region;
    status = aspace_region_reserve(aspace, stack_base, BH_USER_STACK_DEFAULT_SIZE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER, VM_MAP_FIXED, VM_INHERIT_NONE, &stack_region);
    if (status != K_OK) { loader_print_fail("STACK_READY", status); goto fail; }
    txn->regions[txn->region_count++] = stack_base;
    for (uint64_t off = 0; off < BH_USER_STACK_DEFAULT_SIZE; off += PAGE_SIZE) {
        if (txn->page_count >= BH_LOADER_MAX_PAGES) { status = K_ERR_NO_RESOURCES; loader_print_fail("STACK_READY", status); goto fail; }
        void *page_ptr = pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO);
        if (!page_ptr) { status = K_ERR_NO_MEMORY; loader_print_fail("STACK_READY", status); goto fail; }
        status = prot_domain_map_region(aspace->prot_domain, stack_base + off, (phys_addr_t)(uintptr_t)page_ptr, PAGE_SIZE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER);
        if (status != K_OK) { pmm_free_page(page_ptr); loader_print_fail("STACK_READY", status); goto fail; }
        txn->pages[txn->page_count++] = (loader_page_t){.va = stack_base + off, .page = page_ptr};
    }
    loader_write("INIT_STAGE: STACK_READY\n");

    uintptr_t startup_va = guard_base - PAGE_SIZE;
    vm_region_t *startup_region;
    status = aspace_region_reserve(aspace, startup_va, PAGE_SIZE, VM_PROT_READ | VM_PROT_USER, VM_MAP_FIXED, VM_INHERIT_NONE, &startup_region);
    if (status != K_OK) { loader_print_fail("STARTUP_READY", status); goto fail; }
    txn->regions[txn->region_count++] = startup_va;
    void *startup_phys = pmm_alloc_page_ex(MEM_NORMAL, PMM_ALLOC_ZERO);
    if (!startup_phys) { status = K_ERR_NO_MEMORY; loader_print_fail("STARTUP_READY", status); goto fail; }
    void *startup_kvirt = physmap_phys_to_virt((phys_addr_t)(uintptr_t)startup_phys);
    if (!startup_kvirt) { pmm_free_page(startup_phys); status = K_ERR_VM_UNMAPPED; loader_print_fail("STARTUP_READY", status); goto fail; }
    bharat_user_startup_t *startup = (bharat_user_startup_t *)startup_kvirt;
    startup->abi_version = 1;
    startup->struct_size = sizeof(bharat_user_startup_t);
    startup->argc = 0;
    startup->flags = 0;
    startup->argv = 0;
    startup->envp = 0;
    startup->bootstrap.abi_version = 1;
    startup->bootstrap.struct_size = sizeof(bharat_bootstrap_info_t);
    startup->bootstrap.boot_session_id = 0x12345678;
    startup->bootstrap.kernel_instance_id = 0;
    startup->bootstrap.home_core_id = hal_cpu_get_id();
    startup->bootstrap.available_kernel_mask = (1ULL << 0);
    startup->bootstrap.online_core_mask = (1ULL << hal_cpu_get_id());
    startup->bootstrap.self_process_cap = 0;
    startup->bootstrap.bootstrap_cap = 0;
    status = prot_domain_map_region(aspace->prot_domain, startup_va, (phys_addr_t)(uintptr_t)startup_phys, PAGE_SIZE, VM_PROT_READ | VM_PROT_USER);
    if (status != K_OK) { pmm_free_page(startup_phys); loader_print_fail("STARTUP_READY", status); goto fail; }
    txn->pages[txn->page_count++] = (loader_page_t){.va = startup_va, .page = startup_phys};
    loader_write("INIT_STAGE: STARTUP_READY\n");

    out->entry_point = plan.entry_point;
    out->user_stack_top = stack_top;
    out->startup_va = startup_va;
    out->aspace = aspace;
    kfree(txn);
    return K_OK;

fail:
    loader_txn_rollback(txn);
    kfree(txn);
    *out = (bh_user_image_result_t){0};
    return status;
}
