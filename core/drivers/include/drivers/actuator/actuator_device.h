#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ACTUATOR_STATE_DISARMED = 0,
    ACTUATOR_STATE_ARMED,
    ACTUATOR_STATE_FAULTED,
    ACTUATOR_STATE_SAFE
} actuator_state_t;

typedef struct {
    float min_value;
    float max_value;
    float safe_value;
    float max_slew_per_sec;
    uint32_t deadman_timeout_ms;
} actuator_limits_t;

struct actuator_device;
typedef struct actuator_device actuator_device_t;

typedef struct {
    int (*arm)(actuator_device_t* dev);
    int (*disarm)(actuator_device_t* dev);
    int (*set_output)(actuator_device_t* dev, float value);
    int (*get_state)(actuator_device_t* dev, actuator_state_t* out);
    int (*enter_safe_state)(actuator_device_t* dev);
} actuator_device_ops_t;

struct actuator_device {
    const char* name;
    uint32_t actuator_id;
    actuator_limits_t limits;
    const actuator_device_ops_t* ops;
    void* priv;
};

int actuator_core_register(actuator_device_t* dev);
int actuator_core_unregister(actuator_device_t* dev);
actuator_device_t* actuator_core_get(uint32_t actuator_id);
