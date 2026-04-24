#include <sched/cpu_partition.h>
#include <profile/execution_mode.h>
#include <assert.h>
#include <stdio.h>

static void test_single_core_fallback(void) {
    bharat_execution_config_t config = {0};

    // Test: Mixed critical on 1 CPU degrades to temporal partitioning
    config.system_profile = BHARAT_SYSTEM_PROFILE_AUTOMOBILE;
    config.execution_mode = BHARAT_EXEC_MODE_MIXED_CRITICAL;
    config.active_cpu_count = 1;

    int rc = cpu_partition_init(&config);
    assert(rc == 0);

    // Should resolve to temporal
    assert(config.partition_strategy == BHARAT_PARTITION_STRATEGY_TEMPORAL);

    // CPU0 should have SYSTEM role
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);

    // CPU0 should allow all necessary classes in mixed critical
    assert((config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_SYSTEM) != 0);
    assert((config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_FIFO_RT) != 0);
    assert((config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_FAIR) != 0);

    printf("PASS: test_single_core_fallback\n");
}

int main(void) {
    test_single_core_fallback();
    return 0;
}
