#include "trap/syscall_stats.h"
#include <bharat/cpu_local.h>

// Assuming max CPUs
static bh_syscall_stats_t g_per_core_stats[MAX_CPUS];

void bh_syscall_stats_inc_total(uint32_t core_id) {
    if (core_id < MAX_CPUS) g_per_core_stats[core_id].total_calls++;
}

void bh_syscall_stats_inc_fast(uint32_t core_id) {
    if (core_id < MAX_CPUS) g_per_core_stats[core_id].fast_calls++;
}

void bh_syscall_stats_inc_slow(uint32_t core_id) {
    if (core_id < MAX_CPUS) g_per_core_stats[core_id].slow_calls++;
}
