#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stack/can/can_filter.h"
#include "stack/can/can_frame.h"

#define CAN_CTRL_MAX_CONTROLLERS 8U
#define CAN_CTRL_MAX_FILTERS 32U
#define CAN_CTRL_MAX_TX_QUEUE 64U
#define CAN_CTRL_MAX_RX_QUEUE 64U

typedef enum {
    CAN_CTRL_STOPPED = 0,
    CAN_CTRL_ERROR_ACTIVE,
    CAN_CTRL_ERROR_PASSIVE,
    CAN_CTRL_BUS_OFF,
    CAN_CTRL_RECOVERING
} can_controller_state_t;

typedef struct {
    bool classical_can;
    bool can_fd;
    bool loopback;
    bool listen_only;
    uint32_t min_bitrate;
    uint32_t max_bitrate;
    uint32_t min_data_bitrate;
    uint32_t max_data_bitrate;
} can_controller_caps_t;

typedef struct {
    uint64_t rx_frames;
    uint64_t tx_frames;
    uint64_t rx_drops;
    uint64_t tx_drops;
    uint64_t bus_off_count;
    uint64_t recoveries;
    uint64_t error_passive_count;
    uint64_t arbitration_lost_count;
    uint64_t rx_overrun_count;
    uint64_t error_frames;
} can_controller_stats_t;

struct can_controller;
typedef struct can_controller can_controller_t;

typedef struct {
    int (*probe)(can_controller_t* ctrl, void* bus_device);
    int (*init)(can_controller_t* ctrl);
    int (*start)(can_controller_t* ctrl);
    int (*stop)(can_controller_t* ctrl);
    int (*transmit)(can_controller_t* ctrl, const can_frame_t* frame);
    int (*receive)(can_controller_t* ctrl, can_frame_t* out_frame);
    int (*poll)(can_controller_t* ctrl);
    int (*irq)(can_controller_t* ctrl, uint32_t irq_status);
    int (*set_filters)(can_controller_t* ctrl, const can_filter_t* filters, size_t count);
    int (*set_bitrate)(can_controller_t* ctrl, uint32_t nominal_bitrate, uint32_t data_bitrate);
    int (*set_loopback)(can_controller_t* ctrl, bool enabled);
    int (*set_listen_only)(can_controller_t* ctrl, bool enabled);
    int (*get_state)(can_controller_t* ctrl, can_controller_state_t* out_state);
    int (*get_stats)(can_controller_t* ctrl, can_controller_stats_t* out_stats);
    int (*recover_bus)(can_controller_t* ctrl);
} can_controller_ops_t;

struct can_controller {
    const char* name;
    uint8_t controller_id;
    const can_controller_ops_t* ops;
    can_controller_caps_t caps;
    can_controller_state_t state;
    can_controller_stats_t stats;
    uint32_t nominal_bitrate;
    uint32_t data_bitrate;
    bool loopback_enabled;
    bool listen_only_enabled;
    void* bus_ctx;
    void* priv;
};

int can_controller_core_register(can_controller_t* ctrl);
int can_controller_core_unregister(can_controller_t* ctrl);
can_controller_t* can_controller_core_get(uint8_t controller_id);

int can_controller_set_state(can_controller_t* ctrl, can_controller_state_t new_state);
int can_controller_set_bitrate(can_controller_t* ctrl, uint32_t nominal_bitrate, uint32_t data_bitrate);
int can_controller_set_filters(can_controller_t* ctrl, const can_filter_t* filters, size_t count);
int can_controller_enqueue_tx(can_controller_t* ctrl, const can_frame_t* frame);
int can_controller_dequeue_rx(can_controller_t* ctrl, can_frame_t* out_frame);
int can_controller_push_rx(can_controller_t* ctrl, const can_frame_t* frame);
int can_controller_set_loopback(can_controller_t* ctrl, bool enabled);
int can_controller_set_listen_only(can_controller_t* ctrl, bool enabled);
int can_controller_report_error(can_controller_t* ctrl, can_controller_state_t new_state);
int can_controller_validate_frame(const can_frame_t* frame, bool allow_fd);
