#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t pwm_hz;
    uint32_t deadtime_ns;
    bool complementary_pwm;
} motor_pwm_config_t;

typedef struct {
    int (*configure_pwm)(void* ctx, const motor_pwm_config_t* cfg);
    int (*set_duty)(void* ctx, float duty_cycle);
    int (*enable)(void* ctx);
    int (*disable)(void* ctx);
    int (*trip_fault)(void* ctx);
} motor_control_ops_t;

struct motor_device;
typedef struct motor_device motor_device_t;

struct motor_device {
    const char* name;
    uint32_t motor_id;
    const motor_control_ops_t* ops;
    void* priv;
};

int motor_core_register(motor_device_t* dev);
int motor_core_unregister(motor_device_t* dev);
motor_device_t* motor_core_get(uint32_t motor_id);
