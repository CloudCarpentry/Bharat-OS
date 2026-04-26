#include <sched/cpu_partition.h>
#include <sched/cpu_partition_validate.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_cpu_partition_bounds(void) {
    printf("Running test_cpu_partition_bounds...\n");
    bharat_execution_config_t config;

    // NULL config
    assert(cpu_partition_init(NULL) == K_ERR_INVALID_ARG);

    // active_cpu_count == 0
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 0;
    assert(cpu_partition_init(&config) == K_ERR_INVALID_ARG);

    // active_cpu_count > BHARAT_MAX_CPU_PARTITIONS
    config.active_cpu_count = BHARAT_MAX_CPU_PARTITIONS + 1;
    assert(cpu_partition_init(&config) == K_ERR_INVALID_ARG);
}

void test_cpu_partition_single_core(void) {
    printf("Running test_cpu_partition_single_core...\n");
    bharat_execution_config_t config;
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 1;
    config.execution_mode = BHARAT_EXEC_MODE_GENERAL_PURPOSE;

    assert(cpu_partition_init(&config) == K_OK);
    assert(config.partition_strategy == BHARAT_PARTITION_STRATEGY_TEMPORAL);
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_SYSTEM);
    assert(config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_FIFO_RT);
    assert(config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_FAIR);
}

void test_cpu_partition_dual_core_mixed(void) {
    printf("Running test_cpu_partition_dual_core_mixed...\n");
    bharat_execution_config_t config;
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 2;
    config.execution_mode = BHARAT_EXEC_MODE_MIXED_CRITICAL;

    assert(cpu_partition_init(&config) == K_OK);
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_BEST_EFFORT);
    assert(config.cpu_partitions[0].allowed_sched_classes & BHARAT_SCHED_CLASS_FIFO_RT);
    assert(config.cpu_partitions[1].allowed_sched_classes == BHARAT_SCHED_CLASS_FAIR);
}

void test_cpu_partition_quad_core_mixed(void) {
    printf("Running test_cpu_partition_quad_core_mixed...\n");
    bharat_execution_config_t config;
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 4;
    config.execution_mode = BHARAT_EXEC_MODE_MIXED_CRITICAL;

    assert(cpu_partition_init(&config) == K_OK);
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(config.cpu_partitions[2].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(config.cpu_partitions[3].role == BHARAT_CPU_PARTITION_BEST_EFFORT);
}

void test_cpu_partition_eight_core_mixed(void) {
    printf("Running test_cpu_partition_eight_core_mixed...\n");
    bharat_execution_config_t config;
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 8;
    config.execution_mode = BHARAT_EXEC_MODE_MIXED_CRITICAL;

    assert(cpu_partition_init(&config) == K_OK);
    assert(config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(config.cpu_partitions[2].role == BHARAT_CPU_PARTITION_REALTIME);
    for (int i = 3; i < 8; i++) {
        assert(config.cpu_partitions[i].role == BHARAT_CPU_PARTITION_BEST_EFFORT);
        assert(config.cpu_partitions[i].allowed_sched_classes == BHARAT_SCHED_CLASS_FAIR);
    }
}

void test_cpu_partition_validator_invariants(void) {
    printf("Running test_cpu_partition_validator_invariants...\n");
    bharat_execution_config_t config;
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 2;

    // Case 1: No system CPU
    for (int i = 0; i < 2; i++) {
        config.cpu_partitions[i].role = BHARAT_CPU_PARTITION_REALTIME;
        config.cpu_partitions[i].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT;
    }
    assert(cpu_partition_validate(&config) == K_ERR_BAD_STATE);

    // Case 2: Active CPU with NONE role
    config.cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
    config.cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;
    config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_NONE;
    assert(cpu_partition_validate(&config) == K_ERR_BAD_STATE);

    // Case 3: Non-spare CPU with no classes
    config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_BEST_EFFORT;
    config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_NONE;
    assert(cpu_partition_validate(&config) == K_ERR_BAD_STATE);

    // Case 4: REALTIME mode with >1 CPU but no RT class
    config.execution_mode = BHARAT_EXEC_MODE_REALTIME;
    config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_SYSTEM;
    config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;
    assert(cpu_partition_validate(&config) == K_ERR_BAD_STATE);
}

static bharat_execution_config_t g_test_config;
const bharat_execution_config_t *bharat_execution_mode_get_config(void) {
    return &g_test_config;
}

void test_class_allowance(void) {
    printf("Running test_class_allowance...\n");
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.active_cpu_count = 1;
    g_test_config.cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FAIR;

    // Any match
    assert(cpu_partition_allows_any_class(0, BHARAT_SCHED_CLASS_SYSTEM) == true);
    assert(cpu_partition_allows_any_class(0, BHARAT_SCHED_CLASS_FIFO_RT) == false);
    assert(cpu_partition_allows_any_class(0, BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FIFO_RT) == true);

    // All match
    assert(cpu_partition_allows_all_classes(0, BHARAT_SCHED_CLASS_SYSTEM) == true);
    assert(cpu_partition_allows_all_classes(0, BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FAIR) == true);
    assert(cpu_partition_allows_all_classes(0, BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FIFO_RT) == false);

    // Alias
    assert(cpu_partition_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM | BHARAT_SCHED_CLASS_FIFO_RT) == true);
}

int main(void) {
    test_cpu_partition_bounds();
    test_cpu_partition_single_core();
    test_cpu_partition_dual_core_mixed();
    test_cpu_partition_quad_core_mixed();
    test_cpu_partition_eight_core_mixed();
    test_cpu_partition_validator_invariants();
    test_class_allowance();
    printf("All cpu_partition host tests passed!\n");
    return 0;
}
