#include "../../../include/mm/tlb_internal.h"
#include "console/console_core.h"

void tlb_dump_stats(void) {
    console_write_raw("--- TLB Statistics ---\n", 23);
    for (int i = 0; i < MAX_CPUS; i++) {
        if (g_tlb_cpu_state[i].local_flushes > 0 || g_tlb_cpu_state[i].shootdowns_sent > 0 || g_tlb_cpu_state[i].shootdowns_received > 0) {
            // we'll just print simple strings for now, normally use snprintf
            console_write_raw("CPU ", 4);
            char c = '0' + i;
            console_write_raw(&c, 1);
            console_write_raw(" Local Flushes: ", 16);
            // placeholder print, we will use hal_serial_write_hex if needed later
            console_write_raw("\n", 1);
        }
    }
}
