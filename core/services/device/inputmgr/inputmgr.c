#include "inputmgr/inputmgr.h"

#define BHARAT_MAX_INPUT_DEVS 8

static bharat_input_device_t *g_input_devs[BHARAT_MAX_INPUT_DEVS];
static uint32_t g_num_input_devs = 0;

static bh_input_event_t g_input_ring[INPUTMGR_RING_CAPACITY];
static uint32_t g_ring_head = 0;
static uint32_t g_ring_tail = 0;
static uint32_t g_ring_count = 0;

static bh_inputmgr_counters_t g_counters;

void inputmgr_init(void) {
    g_num_input_devs = 0;
    __builtin_memset(g_input_devs, 0, sizeof(g_input_devs));
    bh_inputmgr_reset();
}

void bh_inputmgr_reset(void) {
    g_ring_head = 0;
    g_ring_tail = 0;
    g_ring_count = 0;
    __builtin_memset(g_input_ring, 0, sizeof(g_input_ring));
    __builtin_memset(&g_counters, 0, sizeof(g_counters));
}

const bh_inputmgr_counters_t *bh_inputmgr_get_counters(void) {
    return &g_counters;
}

int bh_inputmgr_enqueue(uint32_t device_id, uint16_t type, uint16_t code, int32_t value) {
    // Perform coalescing for relative movement
    if (type == 2 && g_ring_count > 0) { // EV_REL
        uint32_t prev_tail = (g_ring_tail + INPUTMGR_RING_CAPACITY - 1) % INPUTMGR_RING_CAPACITY;
        if (g_input_ring[prev_tail].device_id == device_id &&
            g_input_ring[prev_tail].type == type &&
            g_input_ring[prev_tail].code == code) {
            g_input_ring[prev_tail].value += value;
            return 0; // Coalesced successfully
        }
    }

    if (g_ring_count >= INPUTMGR_RING_CAPACITY) {
        g_counters.overflow_count++;
        return -1; // Dropped
    }

    bh_input_event_t ev;
    // Real clock placeholder or monotonic ticks can be simulated
    ev.timestamp_ticks = g_counters.sequence;
    ev.device_id = device_id;
    ev.type = type;
    ev.code = code;
    ev.value = value;
    ev.sequence = (uint32_t)g_counters.sequence++;

    g_input_ring[g_ring_tail] = ev;

    uint32_t next_tail = (g_ring_tail + 1) % INPUTMGR_RING_CAPACITY;
    __atomic_store_n(&g_ring_tail, next_tail, __ATOMIC_RELEASE);
    g_ring_count++;

    if (g_ring_count > g_counters.high_water_mark) {
        g_counters.high_water_mark = g_ring_count;
    }

    return 0;
}

int bh_inputmgr_drain(bh_input_event_t *out_events, int max_events) {
    if (!out_events || max_events <= 0) return 0;

    uint32_t current_tail = __atomic_load_n(&g_ring_tail, __ATOMIC_ACQUIRE);
    int drained = 0;

    while (drained < max_events && g_ring_head != current_tail) {
        out_events[drained] = g_input_ring[g_ring_head];
        g_ring_head = (g_ring_head + 1) % INPUTMGR_RING_CAPACITY;
        g_ring_count--;
        drained++;
    }

    return drained;
}

int bharat_input_register(bharat_input_device_t *dev) {
    if (!dev) {
        return -1;
    }

    if (g_num_input_devs >= BHARAT_MAX_INPUT_DEVS) {
        return -2;
    }

    dev->id = g_num_input_devs;
    g_input_devs[g_num_input_devs++] = dev;

    if (dev->ops && dev->ops->open) {
        dev->ops->open(dev);
    }

    g_counters.reset_count++;
    return 0;
}

void bharat_input_report_event(bharat_input_device_t *dev, uint16_t type, uint16_t code, int32_t value) {
    if (!dev) return;
    bh_inputmgr_enqueue(dev->id, type, code, value);
}

void bharat_input_sync(bharat_input_device_t *dev) {
    if (!dev) return;
    bh_inputmgr_enqueue(dev->id, 0, 0, 0); // EV_SYN == 0
}
