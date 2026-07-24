#include <sched/sched_admission.h>
#include <sched/cpu_partition.h>
#include <profile/execution_mode.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mock the execution mode config getter to control tests
static bharat_execution_config_t mock_config;
static bool use_mock_config = false;

const bharat_execution_config_t* bharat_execution_mode_get_config(void) {
    if (use_mock_config) {
        return &mock_config;
    }
    // Fallback to real if available, though for host tests we usually mock
    return NULL;
}

static void setup_mock(uint32_t cpu_count, bharat_execution_mode_t mode) {
    memset(&mock_config, 0, sizeof(mock_config));
    mock_config.active_cpu_count = cpu_count;
    mock_config.execution_mode = mode;
    int rc = cpu_partition_init(&mock_config);
    assert(rc == 0);
    use_mock_config = true;
}

static void test_one_core_temporal_allows_system_rt_fair(void) {
    setup_mock(1, BHARAT_EXEC_MODE_MIXED_CRITICAL);

    assert(mock_config.partition_strategy == BHARAT_PARTITION_STRATEGY_TEMPORAL);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM) == K_OK);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FIFO_RT) == K_OK);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FAIR) == K_OK);

    uint32_t selected_cpu;
    assert(sched_admission_select_cpu(BHARAT_SCHED_CLASS_SYSTEM, &selected_cpu) == K_OK);
    assert(selected_cpu == 0);
    assert(sched_admission_select_cpu(BHARAT_SCHED_CLASS_FIFO_RT, &selected_cpu) == K_OK);
    assert(selected_cpu == 0);
    assert(sched_admission_select_cpu(BHARAT_SCHED_CLASS_FAIR, &selected_cpu) == K_OK);
    assert(selected_cpu == 0);

    assert(sched_admission_validate_partitions() == K_OK);

    printf("PASS: test_one_core_temporal_allows_system_rt_fair\n");
}

static void test_two_core_gp_maps_both_system_fair(void) {
    setup_mock(2, BHARAT_EXEC_MODE_GENERAL_PURPOSE);

    assert(mock_config.partition_strategy == BHARAT_PARTITION_STRATEGY_SPATIAL);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM) == K_OK);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FAIR) == K_OK);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_SYSTEM) == K_OK);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_FAIR) == K_OK);

    // RT should be rejected in GP mode unless explicitly enabled (it's not in our current init)
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FIFO_RT) == K_ERR_DENIED);

    uint32_t selected_cpu;
    // Selection should be deterministic: first eligible
    assert(sched_admission_select_cpu(BHARAT_SCHED_CLASS_FAIR, &selected_cpu) == K_OK);
    assert(selected_cpu == 0);

    assert(sched_admission_validate_partitions() == K_OK);

    printf("PASS: test_two_core_gp_maps_both_system_fair\n");
}

static void test_two_core_rt_maps_cpu0_system_rt_cpu1_spare(void) {
    setup_mock(2, BHARAT_EXEC_MODE_REALTIME);

    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM) == K_OK);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FIFO_RT) == K_OK);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FAIR) == K_ERR_DENIED);

    // CPU1 is SPARE
    assert(mock_config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_SPARE);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_SYSTEM) == K_ERR_DENIED);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_FIFO_RT) == K_ERR_DENIED);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_FAIR) == K_ERR_DENIED);

    uint32_t selected_cpu;
    assert(sched_admission_select_cpu(BHARAT_SCHED_CLASS_FIFO_RT, &selected_cpu) == K_OK);
    assert(selected_cpu == 0);
    assert(sched_admission_select_cpu(BHARAT_SCHED_CLASS_FAIR, &selected_cpu) == K_ERR_NOT_FOUND);

    assert(sched_admission_validate_partitions() == K_OK);

    printf("PASS: test_two_core_rt_maps_cpu0_system_rt_cpu1_spare\n");
}

static void test_three_core_mix_has_system_rt_and_fair(void) {
    setup_mock(3, BHARAT_EXEC_MODE_MIXED_CRITICAL);

    // CPU0: SYSTEM, CPU1: RT, CPU2: FAIR
    assert(mock_config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(mock_config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(mock_config.cpu_partitions[2].role == BHARAT_CPU_PARTITION_BEST_EFFORT);

    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM) == K_OK);
    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_FIFO_RT) == K_ERR_DENIED);

    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_FIFO_RT) == K_OK);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_SYSTEM) == K_ERR_DENIED);

    assert(sched_admission_allows_class(2, BHARAT_SCHED_CLASS_FAIR) == K_OK);
    assert(sched_admission_allows_class(2, BHARAT_SCHED_CLASS_FIFO_RT) == K_ERR_DENIED);

    assert(sched_admission_validate_partitions() == K_OK);

    printf("PASS: test_three_core_mix_has_system_rt_and_fair\n");
}

static void test_four_core_mixed_has_system_rt_and_fair(void) {
    setup_mock(4, BHARAT_EXEC_MODE_MIXED_CRITICAL);

    assert(mock_config.partition_strategy == BHARAT_PARTITION_STRATEGY_SPATIAL);
    assert(mock_config.cpu_partitions[0].role == BHARAT_CPU_PARTITION_SYSTEM);
    assert(mock_config.cpu_partitions[1].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(mock_config.cpu_partitions[2].role == BHARAT_CPU_PARTITION_REALTIME);
    assert(mock_config.cpu_partitions[3].role == BHARAT_CPU_PARTITION_BEST_EFFORT);

    assert(sched_admission_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM) == K_OK);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_FIFO_RT) == K_OK);
    assert(sched_admission_allows_class(2, BHARAT_SCHED_CLASS_FIFO_RT) == K_OK);
    assert(sched_admission_allows_class(3, BHARAT_SCHED_CLASS_FAIR) == K_OK);

    assert(sched_admission_validate_partitions() == K_OK);

    printf("PASS: test_four_core_mixed_has_system_rt_and_fair\n");
}

static void test_rt_class_rejected_on_fair_only_cpu(void) {
    setup_mock(4, BHARAT_EXEC_MODE_MIXED_CRITICAL);
    assert(sched_admission_allows_class(3, BHARAT_SCHED_CLASS_FIFO_RT) == K_ERR_DENIED);
    printf("PASS: test_rt_class_rejected_on_fair_only_cpu\n");
}

static void test_fair_class_rejected_on_rt_only_cpu(void) {
    setup_mock(4, BHARAT_EXEC_MODE_MIXED_CRITICAL);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_FAIR) == K_ERR_DENIED);
    printf("PASS: test_fair_class_rejected_on_rt_only_cpu\n");
}

static void test_unknown_execution_mode_fails_closed(void) {
    setup_mock(2, BHARAT_EXEC_MODE_GENERAL_PURPOSE);
    mock_config.execution_mode = BHARAT_EXEC_MODE_UNKNOWN;

    assert(sched_admission_validate_partitions() == K_ERR_BAD_STATE);

    printf("PASS: test_unknown_execution_mode_fails_closed\n");
}

static void test_invalid_cpu_id_rejected(void) {
    setup_mock(1, BHARAT_EXEC_MODE_GENERAL_PURPOSE);
    assert(sched_admission_allows_class(1, BHARAT_SCHED_CLASS_SYSTEM) == K_ERR_DENIED);
    printf("PASS: test_invalid_cpu_id_rejected\n");
}

static void test_string_helpers(void) {
    assert(strcmp(sched_admission_mode_to_string(BHARAT_EXEC_MODE_GENERAL_PURPOSE), "GP") == 0);
    assert(strcmp(sched_admission_mode_to_string(BHARAT_EXEC_MODE_REALTIME), "RT") == 0);
    assert(strcmp(sched_admission_mode_to_string(BHARAT_EXEC_MODE_MIXED_CRITICAL), "MIX") == 0);
    assert(strcmp(sched_admission_mode_to_string(BHARAT_EXEC_MODE_UNKNOWN), "UNKNOWN") == 0);

    assert(strcmp(sched_admission_partition_to_string(BHARAT_PARTITION_STRATEGY_SPATIAL), "SPATIAL") == 0);
    assert(strcmp(sched_admission_partition_to_string(BHARAT_PARTITION_STRATEGY_TEMPORAL), "TEMPORAL") == 0);
    assert(strcmp(sched_admission_partition_to_string(BHARAT_PARTITION_STRATEGY_NONE), "UNKNOWN") == 0);

    printf("PASS: test_string_helpers\n");
}

int main(void) {
    test_one_core_temporal_allows_system_rt_fair();
    test_two_core_gp_maps_both_system_fair();
    test_two_core_rt_maps_cpu0_system_rt_cpu1_spare();
    test_three_core_mix_has_system_rt_and_fair();
    test_four_core_mixed_has_system_rt_and_fair();
    test_rt_class_rejected_on_fair_only_cpu();
    test_fair_class_rejected_on_rt_only_cpu();
    test_unknown_execution_mode_fails_closed();
    test_invalid_cpu_id_rejected();
    test_string_helpers();
    return 0;
}
