#include "../../include/tests/ktest.h"
#include "../../include/hal/hal_mpa.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/mm/aspace.h"
#include "console/console_core.h"
#include "trap_api.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static volatile bool page_fault_triggered = false;
static volatile virt_addr_t fault_address = 0;

static virt_addr_t get_test_va(void) {
#if defined(__x86_64__) || defined(__aarch64__)
    // Keep this canonical and in the user half on 48-bit VA configurations.
    return (virt_addr_t)0x0000700000000000ULL;
#elif defined(__riscv) && (__riscv_xlen == 64)
    // Keep this canonical for Sv39/Sv48-style layouts as well.
    return (virt_addr_t)0x0000002000000000ULL;
#else
    // 32-bit targets (arm32/riscv32/x86) use a high user-space VA.
    return (virt_addr_t)0x40000000;
#endif
}

extern int (*g_test_fault_hook)(trap_frame_t *frame, const trap_info_t *info);

static int my_test_fault_hook(trap_frame_t *frame, const trap_info_t *info) {
    if ((info->trap_class == TRAP_CLASS_PAGE_FAULT || info->trap_class == TRAP_CLASS_ACCESS_FAULT) &&
        info->fault_addr == get_test_va()) {
        page_fault_triggered = true;
        fault_address = info->fault_addr;

#if defined(__x86_64__)
        frame->pc += 2; 
#elif defined(__aarch64__) || defined(__riscv)
        frame->pc += 4;
#endif

        return 0; // Recovered!
    }
    return -1; // Not our fault, let it panic
}

static int test_unmapped_user_fault(void) {
    KPRINT("Running Hardware Unmapped User Fault Test...\n");

    phys_addr_t paddr = mm_alloc_page(NUMA_NODE_ANY);
    if (paddr == 0) {
        KPRINT("FAIL: Could not allocate test page\n");
        return -1;
    }

    extern address_space_t kernel_space;
    virt_addr_t test_va = get_test_va();

    // Map the page
    if (mm_vmm_map_page(&kernel_space, test_va, paddr, HAL_PT_FLAG_READ | HAL_PT_FLAG_WRITE) != 0) {
        KPRINT("FAIL: Could not map test page\n");
        mm_free_page(paddr);
        return -1;
    }

    // Now unmap the address explicitly to test the hardware page fault.
    mm_vmm_unmap_page(&kernel_space, test_va);

#if defined(__x86_64__)
    phys_addr_t active_root = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_root));
    active_root &= ~(phys_addr_t)0xFFFU; // Strip PCID bits if present
    if (active_hal_pt && active_hal_pt->unmap_page && active_root != kernel_space.root_pt) {
        (void)active_hal_pt->unmap_page(active_root, test_va, NULL);
    }
#endif

    // Verify via HAL that the page is absent
    phys_addr_t queried_paddr;
    int query_res = -1;
    if (active_hal_pt && active_hal_pt->query_page) {
        query_res = active_hal_pt->query_page(kernel_space.root_pt, test_va, &queried_paddr, NULL);
    }
    if (query_res == 0) {
        KPRINT("FAIL: Page was still mapped\n");
        mm_free_page(paddr);
        return -1;
    }
    KPRINT("PASS: User page is verifiably unmapped in hardware page tables.\n");

    int test_result = 0;

#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv)
    // Register our hook
    g_test_fault_hook = my_test_fault_hook;
    page_fault_triggered = false;

    KPRINT("  Triggering hardware fault...\n");

    uint32_t val = 0;

#if defined(__x86_64__)
    __asm__ volatile (
        "mov (%%rbx), %%eax\n"
        : "=a"(val)
        : "b"(test_va)
        : "memory"
    );
#elif defined(__aarch64__)
    __asm__ volatile (
        "ldr %w0, [%1]\n"
        : "=r"(val)
        : "r"(test_va)
        : "memory"
    );
#elif defined(__riscv)
    __asm__ volatile (
        "lw %0, 0(%1)\n"
        : "=r"(val)
        : "r"(test_va)
        : "memory"
    );
#endif

    (void)val;

    g_test_fault_hook = NULL;

    if (page_fault_triggered && fault_address == test_va) {
        KPRINT("PASS: Hardware page fault correctly triggered and caught.\n");
        test_result = 0;
    } else {
        KPRINT("FAIL: Hardware did not trap the unmapped memory access!\n");
        test_result = -1;
    }
#else
    test_result = 0;
#endif

    mm_free_page(paddr);
    return test_result;
}

// Keep this probe out of normal boot for now: on some x86_64 QEMU setups the
// fault-recovery hook is not reliably reached, causing a reboot loop before
// services/init handoff. It remains available in diagnostic/quick-capable
// modes where fault-path validation is explicitly requested.
REGISTER_BOOT_SELFTEST("hw_unmapped_fault", "memory", test_unmapped_user_fault, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_QUICK, 0, false)
