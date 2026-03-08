#include "security/audit.h"

#define BHARAT_AUDIT_RING_SIZE 128U

static bharat_audit_event_t g_audit_ring[BHARAT_AUDIT_RING_SIZE];
static uint32_t g_audit_head = 0U;
static uint64_t g_audit_tick = 0U;

int bharat_audit_init(void) {
    g_audit_head = 0U;
    g_audit_tick = 0U;
    return 0;
}

int bharat_audit_record(bharat_audit_event_type_t type,
                        uint32_t process_id,
                        int32_t result,
                        uint64_t arg0,
                        uint64_t arg1) {
    bharat_audit_event_t* e = &g_audit_ring[g_audit_head % BHARAT_AUDIT_RING_SIZE];
    e->tick = ++g_audit_tick;
    e->type = type;
    e->process_id = process_id;
    e->result = result;
    e->arg0 = arg0;
    e->arg1 = arg1;
    g_audit_head++;
    return 0;
}

int bharat_audit_latest(bharat_audit_event_t* out_event) {
    uint32_t idx;
    if (!out_event || g_audit_head == 0U) {
        return -1;
    }

    idx = (g_audit_head - 1U) % BHARAT_AUDIT_RING_SIZE;
    *out_event = g_audit_ring[idx];
    return 0;
}
