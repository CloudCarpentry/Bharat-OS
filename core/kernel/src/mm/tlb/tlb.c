#include "../../include/mm/tlb.h"
#include "../../include/mm/tlb_internal.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/bharat/cpu_local.h"
#include "arch/arch_caps.h"
#include "hal/hal_timer.h"

tlb_cpu_state_t g_tlb_cpu_state[MAX_CPUS];

int tlb_init(void) {
    extern void tlb_pending_init(void);
    tlb_pending_init();

#ifndef BHARAT_HOST_TEST
    extern void kernel_panic(const char *message);
    if (arch_has_cap(ARCH_CAP_SMP)) {
        if (hal_timer_read_freq() == 0) {
            kernel_panic("TLB: Monotonic timer unavailable with SMP TLB shootdown enabled!");
        }
    }
#endif

    for (int i = 0; i < MAX_CPUS; i++) {
        g_tlb_cpu_state[i].active_aspace = NULL;
        g_tlb_cpu_state[i].active_asid = 0;
        g_tlb_cpu_state[i].last_seen_gen = 0;
        g_tlb_cpu_state[i].shootdowns_received = 0;
        g_tlb_cpu_state[i].local_flushes = 0;
        g_tlb_cpu_state[i].shootdowns_sent = 0;
        g_tlb_cpu_state[i].full_flushes = 0;
        g_tlb_cpu_state[i].aspace_flushes = 0;
        g_tlb_cpu_state[i].range_flushes = 0;
        g_tlb_cpu_state[i].page_flushes = 0;
    }
    return 0;
}

// Legacy helper for single page shootdown
void tlb_shootdown(vm_aspace_t *as, uint64_t vaddr) {
    tlb_invalidate_all(as, vaddr, PAGE_SIZE, TLB_INV_PAGE);
}
