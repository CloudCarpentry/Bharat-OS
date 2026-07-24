#include <sched/cpu_partition.h>
#include <sched/cpu_partition_validate.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bharat_execution_config_t g_test_config;
const bharat_execution_config_t *bharat_execution_mode_get_config(void) {
    return &g_test_config;
}

void test_cpu_partition_helpers(void) {
    printf("Running test_cpu_partition_helpers...\n");
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.active_cpu_count = 2;

    g_test_config.cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
    g_test_config.cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;
    g_test_config.cpu_partitions[0].housekeeping = true;
    g_test_config.cpu_partitions[0].irq_preferred = true;
    g_test_config.cpu_partitions[0].allow_migration_in = true;
    g_test_config.cpu_partitions[0].allow_migration_out = false;

    g_test_config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_REALTIME;
    g_test_config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT;
    g_test_config.cpu_partitions[1].housekeeping = false;
    g_test_config.cpu_partitions[1].irq_preferred = false;
    g_test_config.cpu_partitions[1].allow_migration_in = false;
    g_test_config.cpu_partitions[1].allow_migration_out = true;

    // test_housekeeping
    assert(cpu_partition_is_housekeeping(0) == true);
    assert(cpu_partition_is_housekeeping(1) == false);

    // test_rt_dedicated
    assert(cpu_partition_is_rt_dedicated(0) == false);
    assert(cpu_partition_is_rt_dedicated(1) == true);

    // test_migration_in
    assert(cpu_partition_can_accept_migration(0, BHARAT_SCHED_CLASS_SYSTEM) == true);
    assert(cpu_partition_can_accept_migration(1, BHARAT_SCHED_CLASS_FIFO_RT) == false);

    // test_migration_out
    assert(cpu_partition_can_send_migration(0, BHARAT_SCHED_CLASS_SYSTEM) == false);
    assert(cpu_partition_can_send_migration(1, BHARAT_SCHED_CLASS_FIFO_RT) == true);

    // test_irq_preferred
    assert(cpu_partition_should_receive_irq(0) == true);
    assert(cpu_partition_should_receive_irq(1) == false);

    // invalid cpu
    assert(cpu_partition_get(2) == NULL);
    assert(cpu_partition_is_housekeeping(2) == false);
}

void test_cpu_partition_invariants_extra(void) {
    printf("Running test_cpu_partition_invariants_extra...\n");
    bharat_execution_config_t config;
    memset(&config, 0, sizeof(config));
    config.active_cpu_count = 1;
    config.cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
    config.cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;

    // Case: inactive CPU has non-NONE role
    config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_BEST_EFFORT;
    config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_NONE;
    assert(cpu_partition_validate(&config) == K_ERR_BAD_STATE);

    // Case: inactive CPU has sched classes
    config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_NONE;
    config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FAIR;
    assert(cpu_partition_validate(&config) == K_ERR_BAD_STATE);

    // Reset and confirm OK
    config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_NONE;
    assert(cpu_partition_validate(&config) == K_OK);
}

void test_cpu_partition_admission(void) {
    printf("Running test_cpu_partition_admission...\n");
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.active_cpu_count = 2;

    // CPU0: SYSTEM only
    g_test_config.cpu_partitions[0].role = BHARAT_CPU_PARTITION_SYSTEM;
    g_test_config.cpu_partitions[0].allowed_sched_classes = BHARAT_SCHED_CLASS_SYSTEM;

    // CPU1: RT only
    g_test_config.cpu_partitions[1].role = BHARAT_CPU_PARTITION_REALTIME;
    g_test_config.cpu_partitions[1].allowed_sched_classes = BHARAT_SCHED_CLASS_FIFO_RT;

    assert(cpu_partition_allows_class(0, BHARAT_SCHED_CLASS_SYSTEM) == true);
    assert(cpu_partition_allows_class(0, BHARAT_SCHED_CLASS_FAIR) == false);
    assert(cpu_partition_allows_class(1, BHARAT_SCHED_CLASS_FIFO_RT) == true);
    assert(cpu_partition_allows_class(1, BHARAT_SCHED_CLASS_SYSTEM) == false);
}

int main(void) {
    test_cpu_partition_helpers();
    test_cpu_partition_invariants_extra();
    test_cpu_partition_admission();
    printf("All cpu_partition_validate host tests passed!\n");
    return 0;
}
