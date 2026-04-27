#include <bharat/uapi/safety/bh_watchdog.h>
#include <trace/bh_fast_trace.h>

#define MAX_WATCHED_TASKS 8

static bh_watchdog_heartbeat_t g_heartbeats[MAX_WATCHED_TASKS];
static uint32_t g_num_watched_tasks = 0;

void bh_watchdog_init(void) {
    g_num_watched_tasks = 0;
}

int bh_watchdog_register(uint32_t task_id, uint64_t period_ns) {
    if (g_num_watched_tasks >= MAX_WATCHED_TASKS) return -1;

    g_heartbeats[g_num_watched_tasks].task_id = task_id;
    g_heartbeats[g_num_watched_tasks].period_ns = period_ns;
    g_heartbeats[g_num_watched_tasks].last_seen_ns = 0; // Should be current time
    g_heartbeats[g_num_watched_tasks].missed_count = 0;

    return (int)g_num_watched_tasks++;
}

void bh_watchdog_heartbeat(uint32_t task_id, uint64_t current_ns) {
    for (uint32_t i = 0; i < g_num_watched_tasks; i++) {
        if (g_heartbeats[i].task_id == task_id) {
            g_heartbeats[i].last_seen_ns = current_ns;
            bh_fast_trace_emit(BH_TRACE_AUTO_WATCHDOG_HEARTBEAT, task_id, 0, 0);
            return;
        }
    }
}

bh_watchdog_status_t bh_watchdog_check(uint64_t current_ns) {
    bh_watchdog_status_t worst_status = BH_WATCHDOG_OK;
    for (uint32_t i = 0; i < g_num_watched_tasks; i++) {
        uint64_t elapsed = current_ns - g_heartbeats[i].last_seen_ns;
        if (elapsed > g_heartbeats[i].period_ns * 2) {
            g_heartbeats[i].missed_count++;
            bh_fast_trace_emit(BH_TRACE_AUTO_WATCHDOG_MISS, g_heartbeats[i].task_id, g_heartbeats[i].missed_count, 0);
            worst_status = BH_WATCHDOG_EXPIRED;
        } else if (elapsed > g_heartbeats[i].period_ns) {
            worst_status = BH_WATCHDOG_LATE;
        }
    }
    return worst_status;
}
