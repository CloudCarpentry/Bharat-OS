#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include "drivers/actuator/actuator_device.h"

// MAX_ACTUATORS is 32 in actuator_core.c, but we can test up to MAX_ACTUATORS by registering and unregistering many times.
#define MAX_ACTUATORS 32
#define ITERATIONS 10000000

int dummy_arm(actuator_device_t* dev) { return 0; }
int dummy_disarm(actuator_device_t* dev) { return 0; }

actuator_device_t devs[MAX_ACTUATORS];
actuator_device_ops_t ops = { .arm = dummy_arm, .disarm = dummy_disarm };

void setup() {
    for (int i = 0; i < MAX_ACTUATORS; i++) {
        devs[i].actuator_id = i + 1;
        devs[i].ops = &ops;
    }
}

int main() {
    setup();

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int it = 0; it < ITERATIONS; it++) {
        // Register all
        for (int i = 0; i < MAX_ACTUATORS; i++) {
            actuator_core_register(&devs[i]);
        }
        // Unregister all from the beginning (worst case for shifting)
        for (int i = 0; i < MAX_ACTUATORS; i++) {
            actuator_core_unregister(&devs[i]);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Benchmark time: %.6f seconds\n", elapsed);

    return 0;
}
