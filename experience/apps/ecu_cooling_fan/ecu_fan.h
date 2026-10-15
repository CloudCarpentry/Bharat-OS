#ifndef BHARAT_ECU_COOLING_FAN_H
#define BHARAT_ECU_COOLING_FAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Operational states for the Cooling Fan ECU */
typedef enum {
    ECU_STATE_UNINITIALIZED = 0,
    ECU_STATE_NORMAL        = 1,
    ECU_STATE_HOT           = 2,
    ECU_STATE_COOLING       = 3,
    ECU_STATE_SAFE_STATE    = 4
} ecu_fan_state_t;

/* Actuator request */
typedef enum {
    ECU_FAN_CMD_OFF = 0,
    ECU_FAN_CMD_ON  = 1
} ecu_fan_cmd_t;

/* Sensor sample with status */
typedef struct {
    int32_t temp_celsius;
    bool is_valid;
    uint32_t timestamp_ms;
} ecu_sensor_sample_t;

/* Static configuration (thresholds in Celsius) */
typedef struct {
    int32_t temp_on_threshold;   /* e.g. 85°C: turn fan ON */
    int32_t temp_off_threshold;  /* e.g. 65°C: turn fan OFF */
    int32_t min_valid_temp;      /* e.g. -40°C */
    int32_t max_valid_temp;      /* e.g. 150°C */
    uint32_t max_consecutive_faults;
} ecu_fan_config_t;

/* ECU Runtime Controller Context (bounded static memory) */
typedef struct {
    ecu_fan_config_t config;
    ecu_fan_state_t current_state;
    ecu_fan_cmd_t fan_command;
    int32_t last_valid_temp;
    uint32_t fault_count;
    uint32_t deadline_miss_count;
    uint32_t total_samples_processed;
    uint32_t last_sample_time_ms;
} ecu_fan_controller_t;

/* Callback interface for serial/diagnostic logging */
typedef void (*ecu_log_fn_t)(const char *message);

/* Initialize the ECU fan controller */
void ecu_fan_init(ecu_fan_controller_t *ctrl, const ecu_fan_config_t *config, ecu_log_fn_t logger);

/* Process a single temperature sample and update state machine */
void ecu_fan_process_sample(ecu_fan_controller_t *ctrl, const ecu_sensor_sample_t *sample);

/* Check if the actuator request is currently ON */
bool ecu_fan_is_active(const ecu_fan_controller_t *ctrl);

/* Get string representation of ECU state */
const char *ecu_fan_state_name(ecu_fan_state_t state);

/* Execute full deterministic reference validation scenario */
bool ecu_fan_run_scenario_test(ecu_fan_controller_t *ctrl);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_ECU_COOLING_FAN_H */
