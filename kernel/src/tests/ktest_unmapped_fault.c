#include "../../include/tests/ktest.h"
#include "../../include/hal/hal_mpa.h"
#include "../../include/hal/mmu_ops.h"
#include "../../include/mm/aspace.h"
#include "console/console_core.h"
#include "trap_api.h"

#define KPRINT(s) console_write_raw(s, string_length(s))

static volatile bool page_fault_triggered = false;
static volatile virt_addr_t fault_address = 0;

#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv)
// If we can't easily hook the exception handler without disrupting the system,
// testing a real hardware fault inline inside the kernel requires an exception
// table mechanism which we might not have in the test suite yet.
// However, the test only requires proving that the HAL mapping correctly modifies
// hardware state so a user-space access would fault.

// Let's implement an inline assembly test.
// Wait, if it faults, the kernel's generic page fault handler (`trap_dispatch` -> `vmm_handle_cow_fault` -> `trap_handle_fault`)
// will be invoked. In the kernel boot tests, a panic usually halts the machine.
// Can we override the weak `trap_handle_fault`? Yes, tests often do.
// But we are compiled into the core kernel, so `trap_handle_fault` is probably implemented in `arch/x86_64/trap.c` or similar.
#endif

// We'll declare a global function pointer that the real `trap_handle_fault` can call if it's a test.
extern int (*g_test_fault_hook)(trap_frame_t *frame, const trap_info_t *info);

static int my_test_fault_hook(trap_frame_t *frame, const trap_info_t *info) {
    if (info->trap_class == TRAP_CLASS_PAGE_FAULT || info->trap_class == TRAP_CLASS_ACCESS_FAULT) {
        page_fault_triggered = true;
        fault_address = info->fault_addr;

        // Advance the instruction pointer to skip the faulting instruction.
        // Assuming a simple 32-bit instruction (e.g. RISC-V or Aarch64).
        // On x86, instruction length varies, but for a simple read we can just add 3 or 4 bytes,
        // or we can just return from the test entirely if the architecture supports it.
        // The safest generic way without full disassembler is to advance PC slightly or jump.
        // Since we don't have setjmp/longjmp in this freestanding kernel test, we will
        // advance the PC by a fixed amount.

#if defined(__x86_64__)
        frame->pc += 2; // Exact length of `mov (%rbx), %eax` instruction generated
#elif defined(__aarch64__) || defined(__riscv)
        frame->pc += 4;
#endif

        return 0; // Recovered!
    }
    return -1; // Not our fault, let it panic
}

static int test_unmapped_user_fault(void) {
    KPRINT("Running Hardware Unmapped User Fault Test...\n");

    virt_addr_t test_va = 0x400000;

    // Unmap the address explicitly
    extern address_space_t kernel_space;
    mm_vmm_unmap_page(&kernel_space, test_va);

    // Verify via HAL that the page is absent
    phys_addr_t paddr;
    int query_res = -1;
    if (active_mem_protect && active_mem_protect->cpu_ops.unmap_page) {
        query_res = active_mem_protect->cpu_ops.unmap_page(kernel_space.root_pt, test_va, &paddr);
    }
    if (query_res == 0) {
        KPRINT("FAIL: Page was still mapped\n");
        return -1;
    }
    KPRINT("PASS: User page is verifiably unmapped in hardware page tables.\n");

#if defined(__x86_64__) || defined(__aarch64__) || defined(__riscv)
    // Register our hook
    g_test_fault_hook = my_test_fault_hook;
    page_fault_triggered = false;

    KPRINT("  Triggering hardware fault...\n");

    // We are in the try block. Force a read from the unmapped address.
    // Because the address is in the lower half (user space), and we are in ring 0 (kernel),
    // if SMAP/PAN is disabled, it might succeed if mapped. But we just unmapped it,
    // so it *will* trigger a page fault in hardware because the PTE is not present.
    // To ensure exact instruction length for the PC advance, we use inline assembly.

    uint32_t val = 0;

#if defined(__x86_64__)
    // mov eax, [test_va] is 3 bytes with a simple register, but accessing an immediate 64-bit addr
    // can be longer. We pass test_va in a register.
    // `mov (%rbx), %eax` is 2 bytes: 8B 03.
    // We adjust the trap handler PC += 2 for x86_64.
    __asm__ volatile (
        "mov (%%rbx), %%eax\n"
        : "=a"(val)
        : "b"(test_va)
        : "memory"
    );
#elif defined(__aarch64__)
    // ldr w0, [x1] is 4 bytes. Handler advances PC by 4.
    __asm__ volatile (
        "ldr %w0, [%1]\n"
        : "=r"(val)
        : "r"(test_va)
        : "memory"
    );
#elif defined(__riscv)
    // lw a0, 0(a1) is 4 bytes. Handler advances PC by 4.
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
        return 0;
    } else {
        KPRINT("FAIL: Hardware did not trap the unmapped memory access!\n");
        return -1;
    }
#else
    return 0;
#endif
}

REGISTER_BOOT_SELFTEST("hw_unmapped_fault", "memory", test_unmapped_user_fault, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false)
