#include "../../../include/mm/tlb_internal.h"
#include "../../../include/mm/tlb.h"
#include "console/console_core.h"

static tlb_failure_snapshot_t s_tlb_last_failures[MAX_CPUS];

void tlb_dump_stats(void) {
    console_write_raw("--- TLB Statistics ---\n", 23);
    for (int i = 0; i < MAX_CPUS; i++) {
        if (g_tlb_cpu_state[i].local_flushes > 0 || g_tlb_cpu_state[i].shootdowns_sent > 0 || g_tlb_cpu_state[i].shootdowns_received > 0) {
            console_write_raw("CPU ", 4);
            char c = '0' + i;
            console_write_raw(&c, 1);
            console_write_raw(" Local Flushes: ", 16);
            console_write_raw("\n", 1);
        }
    }
}

kstatus_t tlb_diag_get_last_failure(uint32_t cpu, tlb_failure_snapshot_t *out) {
    if (cpu >= MAX_CPUS || !out) return K_ERR_INVALID_ARG;
    *out = s_tlb_last_failures[cpu];
    return K_OK;
}

void tlb_diag_set_last_failure(uint32_t cpu, const tlb_failure_snapshot_t *in) {
    if (cpu < MAX_CPUS && in) {
        s_tlb_last_failures[cpu] = *in;
    }
}
