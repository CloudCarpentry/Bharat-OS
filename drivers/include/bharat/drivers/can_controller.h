#ifndef BHARAT_DRIVERS_CAN_CONTROLLER_H
#define BHARAT_DRIVERS_CAN_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

/* CAN Frame structure, ready for CAN FD */
#define CAN_MAX_DLC      8U
#define CANFD_MAX_DLC    64U

typedef enum {
    CAN_ID_TYPE_STANDARD = 0, /* 11-bit identifier */
    CAN_ID_TYPE_EXTENDED = 1  /* 29-bit identifier */
} can_id_type_t;

typedef struct {
    uint32_t      id;
    can_id_type_t id_type;
    uint8_t       dlc;
    bool          is_fd;
    bool          brs;      /* Bit Rate Switch (CAN FD only) */
    bool          esi;      /* Error State Indicator (CAN FD only) */
    bool          is_rtr;   /* Remote Transmission Request */
    uint64_t      timestamp;/* Nanoseconds or controller-specific ticks */
    uint8_t       payload[CANFD_MAX_DLC];
} can_frame_t;

/* CAN Controller State */
typedef enum {
    CAN_STATE_STOPPED,
    CAN_STATE_ERROR_ACTIVE,
    CAN_STATE_ERROR_PASSIVE,
    CAN_STATE_BUS_OFF,
    CAN_STATE_RECOVERING
} can_controller_state_t;

/* CAN Hardware Filter Type */
typedef enum {
    CAN_FILTER_EXACT,
    CAN_FILTER_MASK
} can_filter_type_t;

typedef struct {
    can_filter_type_t type;
    can_id_type_t     id_type;
    uint32_t          id;
    uint32_t          mask; /* Only used if type == CAN_FILTER_MASK */
} can_hardware_filter_t;

/* Controller Statistics */
typedef struct {
    uint32_t tx_frames;
    uint32_t rx_frames;
    uint32_t tx_errors;
    uint32_t rx_errors;
    uint32_t rx_dropped;
    uint32_t bus_off_count;
    uint32_t arb_lost_count;
} can_controller_stats_t;

/* Abstract CAN Controller Operations (vtable) */
struct can_controller;
typedef struct can_controller can_controller_t;

typedef struct {
    /* Initialize hardware, but keep it stopped */
    int (*init)(can_controller_t* ctrl);

    /* Start controller (join bus) */
    int (*start)(can_controller_t* ctrl);

    /* Stop controller (leave bus) */
    int (*stop)(can_controller_t* ctrl);

    /* Send a frame. Non-blocking. Returns 0 on success, -ENOSPC if Tx queue full. */
    int (*send)(can_controller_t* ctrl, const can_frame_t* frame);

    /* Receive a frame. Non-blocking. Returns 0 if frame read, -ENOENT if empty. */
    int (*recv)(can_controller_t* ctrl, can_frame_t* frame);

    /* Setup a hardware acceptance filter. */
    int (*add_filter)(can_controller_t* ctrl, const can_hardware_filter_t* filter, uint32_t* filter_id);

    /* Remove a hardware acceptance filter. */
    int (*remove_filter)(can_controller_t* ctrl, uint32_t filter_id);

    /* Enable loopback mode */
    int (*set_loopback)(can_controller_t* ctrl, bool enable);

    /* Enable listen-only mode */
    int (*set_listen_only)(can_controller_t* ctrl, bool enable);

    /* Trigger bus-off recovery */
    int (*recover_bus_off)(can_controller_t* ctrl);

    /* Get current state */
    can_controller_state_t (*get_state)(can_controller_t* ctrl);

    /* Get hardware statistics */
    int (*get_stats)(can_controller_t* ctrl, can_controller_stats_t* stats);

} can_controller_ops_t;

/* Base CAN controller structure */
struct can_controller {
    const char*           name;
    const can_controller_ops_t* ops;
    void*                 priv_data; /* Driver-specific private data */
    can_controller_state_t state;
    can_controller_stats_t stats;
};

/* Core generic API */
int can_controller_core_register(can_controller_t* ctrl);
int can_controller_core_unregister(can_controller_t* ctrl);

/* Provide a common way for ISRs to push frames to a ring buffer and signal userspace */
void can_controller_handle_rx_irq(can_controller_t* ctrl, const can_frame_t* frame);
void can_controller_handle_tx_irq(can_controller_t* ctrl);
void can_controller_handle_error_irq(can_controller_t* ctrl, can_controller_state_t new_state);

#endif // BHARAT_DRIVERS_CAN_CONTROLLER_H