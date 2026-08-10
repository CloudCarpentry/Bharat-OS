#include "trace/trace.h"
#include "hal/hal_timer.h"
#include "bharat/cpu_local.h"
#include "console/console_core.h"

// Permanent owner-local trace rings array
// This remains statically allocated, but is explicitly bound one-to-one with
// cpu_local_t and only written by the owning CPU.
static trace_ring_t g_trace_rings[MAX_CPUS] __attribute__((aligned(64)));

void trace_init(void) {
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        g_trace_rings[i].head = 0;
        g_trace_rings[i].tail = 0;
        g_trace_rings[i].dropped = 0;
        // Pointer assigned once during initialization, never re-bound.
        g_cpu_locals[i].trace_ring = &g_trace_rings[i];
    }
}

void trace_emit(trace_event_type_t type, uint64_t a1, uint64_t a2, uint64_t a3) {
    cpu_local_t *cpu = this_cpu();
    if (!cpu || !cpu->trace_ring) return;

    trace_ring_t *ring = (trace_ring_t *)cpu->trace_ring;

    uint32_t head = ring->head;
    uint32_t next = (head + 1) % TRACE_BUFFER_SIZE;

    // Placeholder for timer reading as baremetal HAL doesn't expose get_ms yet
    ring->buffer[head].timestamp = 0;
    ring->buffer[head].type = type;
    ring->buffer[head].arg1 = a1;
    ring->buffer[head].arg2 = a2;
    ring->buffer[head].arg3 = a3;

    ring->head = next;
    if (next == ring->tail) {
        // Buffer full, drop oldest
        ring->tail = (ring->tail + 1) % TRACE_BUFFER_SIZE;
        ring->dropped++;
    }
}

void trace_dump(void) {
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        cpu_local_t *cpu = &g_cpu_locals[i];
        if (!cpu || !cpu->trace_ring) continue;

        trace_ring_t *ring = (trace_ring_t *)cpu->trace_ring;
        uint32_t curr = ring->tail;

        if (curr == ring->head) continue; // Empty ring

        console_log(CONSOLE_LEVEL_INFO, "Trace dump for CPU %u:\n", i);
        if (ring->dropped > 0) {
             console_log(CONSOLE_LEVEL_WARN, "  (Dropped %u events)\n", ring->dropped);
        }

        while(curr != ring->head) {
            trace_event_t *ev = &ring->buffer[curr];
            // normally format to hal_serial_puts, but use neutral console now
            console_log(CONSOLE_LEVEL_INFO, "  Event: type=%d, arg1=%lx, arg2=%lx, arg3=%lx\n",
                        ev->type, ev->arg1, ev->arg2, ev->arg3);
            curr = (curr + 1) % TRACE_BUFFER_SIZE;
        }
    }
}
