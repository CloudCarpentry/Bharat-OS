#include <sched/cpu_partition.h>
#include <profile/execution_mode.h>
#include <assert.h>
#include <stdio.h>

static void test_dual_core_mapping(void) {
    bharat_execution_config_t config = {0};

    // Test: Mixed critical on 2 CPU
    config.system_profile = BHARAT_SYSTEM_PROFILE_MOBILE;
    config.execution_mode = BHARAT_EXEC_MODE_MIXED_CRITICAL;
    config.active_cpu_count = 2;

    int rc = cpu_partition_init(&config);
    assert(rc == 0);

    // Should resolve to spatial strategy for >1 core
    assert(config.partition_strategy == BHARAT_PARTITION_STRATEGY_SPATIAL);

    // CPU0 = SYSTEM + RT
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert((config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_SYSTEM) != 0);
    assert((config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_FIFO_RT) != 0);

    // CPU1 = BEST_EFFORT
    assert(config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_BEST_EFFORT);
    assert((config.cpu_partitions[1].allowed_sched_classes & BHARAT_SCHED_CLASS_FAIR) != 0);
    assert((config.cpu_partitions[1].allowed_sched_classes & BHARAT_SCHED_CLASS_FIFO_RT) == 0);

    printf("PASS: test_dual_core_mapping\n");
}

int main(void) {
    test_dual_core_mapping();
    return 0;
}
