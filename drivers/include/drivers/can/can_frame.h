#pragma once
#include <stdint.h>
#include <stdbool.h>

#define CAN_MAX_DATA_LEN 64

typedef enum {
    CAN_FRAME_DATA = 0,
    CAN_FRAME_REMOTE = 1,
    CAN_FRAME_ERROR = 2,
} can_frame_kind_t;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data_len;
    uint8_t controller_id;
    uint64_t timestamp_ns;
    bool is_extended;
    bool is_fd;
    bool bitrate_switch;
    bool error_state_indicator;
    can_frame_kind_t kind;
    uint8_t data[CAN_MAX_DATA_LEN];
} can_frame_t;
