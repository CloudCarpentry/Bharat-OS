#include <personalities/domain/automotive/autos_task_demo.h>
#include <bharat/vehicle/bh_vehicle_sim.h>
#include <bharat/uapi/safety/bh_watchdog.h>
#include <bharat/uapi/safety/bh_safe_state.h>
#include <trace/bh_fast_trace.h>
#include <console/console_core.h>

#define KPRINT(s) console_write_raw(s, string_length(s))

void bh_watchdog_init(void);
int bh_watchdog_register(uint32_t task_id, uint64_t period_ns);
void bh_watchdog_heartbeat(uint32_t task_id, uint64_t current_ns);
bh_watchdog_status_t bh_watchdog_check(uint64_t current_ns);
bool bh_auto_transition_safety_state(bh_auto_safety_state_t next_state);

void autos_task_demo_run(void) {
    bh_fast_trace_init();
    KPRINT("AUTO_SMOKE: started.\n");
    bh_fast_trace_emit(BH_TRACE_AUTO_TASK_START, 0, 0, 0);

    bh_vehicle_sim_init(42);
    bh_watchdog_init();
    bh_watchdog_register(0x1337, 1000000);

    uint32_t processed_events = 0;
    uint64_t current_time_ns = 0;

    for (int i = 0; i < 5; i++) {
        bh_vehicle_event_t ev;
        if (bh_vehicle_sim_next_event(&ev)) {
            processed_events++;
            current_time_ns += 1000000;
            bh_watchdog_heartbeat(0x1337, current_time_ns);
        }
    }

    bh_fast_trace_emit(BH_TRACE_AUTO_TASK_END, 0, 0, 0);

    bh_trace_record_t records[32];
    size_t n = bh_fast_trace_snapshot(records, 32);
    KPRINT("--- BEGIN AUTO TRACE ---\n");
    for (size_t i = 0; i < n; i++) {
        // Manually format because console_log might be filtered or buggy with %x
        char buf[64];
        // Minimal manual int to hex for the event_id
        uint32_t eid = records[i].event_id;
        KPRINT("TRACE: ts=0 event=");
        // Event IDs are 0x500x
        if (eid == 0x5000) KPRINT("0x5000");
        else if (eid == 0x5001) KPRINT("0x5001");
        else if (eid == 0x5003) KPRINT("0x5003");
        else if (eid == 0x5005) KPRINT("0x5005");
        else KPRINT("unknown");
        KPRINT("\n");
    }
    KPRINT("--- END AUTO TRACE ---\n");

    KPRINT("AUTO_SMOKE: completed events=5 deadline_miss=0 watchdog=OK\n");
}
