#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "drivers/sensor/sensor_device.h"

#define BENCH_SENSORS 10000

int dummy_read(sensor_device_t* dev, sensor_sample_t* out) { return 0; }

sensor_device_ops_t bench_ops = { .read_sample = dummy_read };
sensor_device_t bench_devs[BENCH_SENSORS];

void run_benchmark() {
    clock_t start, end;
    double cpu_time_used;

    printf("Benchmarking sensor registration/unregistration with %d sensors...\n", BENCH_SENSORS);

    // Initialize
    for (int i = 0; i < BENCH_SENSORS; i++) {
        bench_devs[i].sensor_id = i + 1;
        bench_devs[i].ops = &bench_ops;
    }

    start = clock();

    // Register all
    for (int i = 0; i < BENCH_SENSORS; i++) {
        int res = sensor_core_register(&bench_devs[i]);
        if (res != 0) {
            printf("Failed to register sensor %d\n", i);
            exit(1);
        }
    }

    // Unregister from the beginning (worst case for shifting)
    for (int i = 0; i < BENCH_SENSORS; i++) {
        int res = sensor_core_unregister(&bench_devs[i]);
        if (res != 0) {
            printf("Failed to unregister sensor %d\n", i);
            exit(1);
        }
    }

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    printf("Benchmark finished in %f seconds.\n", cpu_time_used);
}

int main() {
    run_benchmark();
    return 0;
}
