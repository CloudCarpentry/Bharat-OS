#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "drivers/motor/motor_control.h"

int dummy_config_pwm(void* ctx, const motor_pwm_config_t* cfg) {
    if (cfg->pwm_hz == 0) return -1;
    return 0;
}
int dummy_set_duty(void* ctx, float duty_cycle) { return 0; }
int dummy_enable(void* ctx) { return 0; }
int dummy_disable(void* ctx) { return 0; }
int dummy_trip(void* ctx) { return 0; }

motor_control_ops_t ops = {
    .configure_pwm = dummy_config_pwm,
    .set_duty = dummy_set_duty,
    .enable = dummy_enable,
    .disable = dummy_disable,
    .trip_fault = dummy_trip
};

void test_motor_registration() {
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

    assert(motor_core_unregister(&dev2) == 0);
    assert(motor_core_get(2) == NULL);
}

// To measure an algorithmic improvement, we'll parameterize the list size
// and measure over a high number of iterations. We use MAX_MOTORS=8 for the benchmark.
void benchmark_unregister() {
    motor_device_t devs[8];
    for (int i = 0; i < 8; i++) {
        devs[i].motor_id = i;
        devs[i].ops = &ops;
    }

    const int iterations = 1000000;

    clock_t start = clock();
    for (int it = 0; it < iterations; it++) {
        // Register all
        for (int i = 0; i < 8; i++) {
            motor_core_register(&devs[i]);
        }
        // Unregister all from the front to maximize shifting in old implementation
        for (int i = 0; i < 8; i++) {
            motor_core_unregister(&devs[i]);
        }
    }
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Benchmark: %d iterations of 8x register/unregister took %f seconds.\n", iterations, time_spent);
}

int main() {
    test_motor_registration();
    printf("test_motor_control_core passed.\n");
    benchmark_unregister();
    return 0;
}
