#include <stdio.h>
#include <assert.h>
#include "drivers/motor/motor_control.h"

int dummy_config_pwm(void* ctx, const motor_pwm_config_t* cfg) {
    if (cfg->pwm_hz == 0) return -1;
    return 0;
}
int dummy_set_duty(void* ctx, float duty_cycle) { return 0; }
int dummy_enable(void* ctx) { return 0; }
int dummy_disable(void* ctx) { return 0; }
int dummy_trip(void* ctx) { return 0; }

void test_motor_registration() {
    motor_control_ops_t ops = {
        .configure_pwm = dummy_config_pwm,
        .set_duty = dummy_set_duty,
        .enable = dummy_enable,
        .disable = dummy_disable,
        .trip_fault = dummy_trip
    };

    motor_device_t dev1 = { .motor_id = 1, .ops = &ops };
    motor_device_t dev2 = { .motor_id = 2, .ops = &ops };

    assert(motor_core_register(&dev1) == 0);
    assert(motor_core_register(&dev2) == 0);

    assert(motor_core_get(1) == &dev1);
    assert(motor_core_get(2) == &dev2);

    // Test invalid config rejected
    motor_pwm_config_t bad_cfg = { .pwm_hz = 0 };
    assert(dev1.ops->configure_pwm(NULL, &bad_cfg) == -1);

    assert(motor_core_unregister(&dev1) == 0);
    assert(motor_core_get(1) == NULL);
}

int main() {
    test_motor_registration();
    printf("test_motor_control_core passed.\n");
    return 0;
}
