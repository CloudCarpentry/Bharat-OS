#include <sched/cpu_partition.h>
#include <profile/execution_mode.h>
#include <assert.h>
#include <stdio.h>

static void test_quad_core_mapping(void) {
    bharat_execution_config_t config = {0};

    // Test: Mixed critical on 4 CPU
    config.system_profile = BHARAT_SYSTEM_PROFILE_AUTOMOBILE;
    config.execution_mode = BHARAT_EXEC_MODE_MIXED_CRITICAL;
    config.active_cpu_count = 4;

    int rc = cpu_partition_init(&config);
    assert(rc == 0);

    // Strategy must be spatial
    assert(config.partition_strategy == BHARAT_PARTITION_STRATEGY_SPATIAL);

    // CPU0 = SYSTEM
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(config.cpu_partitions[0].allowed_sched_classes == BHARAT_SCHED_CLASS_SYSTEM);

    // CPU1 = REALTIME
    assert(config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(config.cpu_partitions[1].allowed_sched_classes == BHARAT_SCHED_CLASS_FIFO_RT);

    // CPU2 = REALTIME
    assert(config.cpu_partitions[2].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(config.cpu_partitions[2].allowed_sched_classes == BHARAT_SCHED_CLASS_FIFO_RT);

    // CPU3 = BEST EFFORT
    assert(config.cpu_partitions[3].role == BHARAT_CPU_PARTITION_BEST_EFFORT);
    assert(config.cpu_partitions[3].allowed_sched_classes == BHARAT_SCHED_CLASS_FAIR);

    printf("PASS: test_quad_core_mapping\n");
}

int main(void) {
    test_quad_core_mapping();
    return 0;
}
