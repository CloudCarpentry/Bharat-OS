#include <bharat/vehicle/bh_vehicle_sim.h>
#include <trace/bh_fast_trace.h>

static uint64_t g_sim_seed = 0;
static uint32_t g_fault_flags = 0;
static uint64_t g_event_counter = 0;

void bh_vehicle_sim_init(uint64_t seed) {
    g_sim_seed = seed;
    g_fault_flags = 0;
    g_event_counter = 0;
}

bool bh_vehicle_sim_next_event(bh_vehicle_event_t *out) {
    if (!out) return false;

    // Deterministic simulation based on seed and counter
    uint64_t state = g_sim_seed + g_event_counter;
    g_event_counter++;

    // Cycle through event types
    uint32_t type_selector = (uint32_t)(state % 6);

    switch (type_selector) {
        case 0:
            out->type = BH_VEHICLE_EVENT_CAN_FRAME;
            out->data.can_frame.bus_id = 0;
            out->data.can_frame.frame_id = 0x100 + (uint32_t)(state % 16);
            out->data.can_frame.dlc = 8;
            for (int i=0; i<8; i++) out->data.can_frame.data[i] = (uint8_t)(state + i);
            out->data.can_frame.timestamp_ns = state * 1000000;
            bh_fast_trace_emit(BH_TRACE_AUTO_CAN_RX, out->data.can_frame.frame_id, 0, 0);
            break;
        case 1:
            out->type = BH_VEHICLE_EVENT_SPEED_SAMPLE;
            out->data.speed_kmh = (int32_t)(state % 120);
            break;
        case 2:
            out->type = BH_VEHICLE_EVENT_BRAKE_SAMPLE;
            out->data.brake_pressure_pct = (uint8_t)(state % 100);
            break;
        case 3:
            out->type = BH_VEHICLE_EVENT_STEERING_SAMPLE;
            out->data.steering_angle_deg = (int16_t)((state % 360) - 180);
            break;
        case 4:
            out->type = BH_VEHICLE_EVENT_POWER_MODE;
            out->data.power_mode = (uint8_t)(state % 5);
            break;
        case 5:
            if (g_fault_flags != 0) {
                out->type = BH_VEHICLE_EVENT_FAULT_INJECTION;
                out->data.fault_flags = g_fault_flags;
            } else {
                // Fallback to CAN frame if no fault
                out->type = BH_VEHICLE_EVENT_CAN_FRAME;
                out->data.can_frame.frame_id = 0x200;
                out->data.can_frame.dlc = 4;
            }
            break;
    }

    return true;
}

void bh_vehicle_sim_set_fault_mode(uint32_t fault_flags) {
    g_fault_flags = fault_flags;
}
