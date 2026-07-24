#ifndef BHARAT_VEHICLE_EVENT_H
#define BHARAT_VEHICLE_EVENT_H
#include <stdint.h>
typedef enum bh_vehicle_event_type {
    BH_VEHICLE_EVENT_CAN_FRAME = 1,
    BH_VEHICLE_EVENT_SPEED_SAMPLE,
    BH_VEHICLE_EVENT_BRAKE_SAMPLE,
    BH_VEHICLE_EVENT_STEERING_SAMPLE,
    BH_VEHICLE_EVENT_POWER_MODE,
    BH_VEHICLE_EVENT_FAULT_INJECTION,
} bh_vehicle_event_type_t;
typedef struct bh_vehicle_can_frame {
    uint32_t bus_id;
    uint32_t frame_id;
    uint8_t  dlc;
    uint8_t  data[8];
    uint64_t timestamp_ns;
} bh_vehicle_can_frame_t;
typedef struct bh_vehicle_event {
    bh_vehicle_event_type_t type;
    union {
        bh_vehicle_can_frame_t can_frame;
        int32_t speed_kmh;
        uint8_t brake_pressure_pct;
        int16_t steering_angle_deg;
        uint32_t fault_flags;
        uint8_t power_mode;
    } data;
} bh_vehicle_event_t;
#endif
