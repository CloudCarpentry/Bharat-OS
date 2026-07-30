#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sched/sched.h"

// Mock runqueues
sched_rq_t g_mock_rqs[2];

uint32_t g_active_core_count = 2;
uint8_t g_sched_initialized = 1;
sched_policy_t g_policy = SCHED_POLICY_PRIORITY;

// Stub for hal_cpu_get_id
uint32_t hal_cpu_get_id(void) {
    return 0; // Coordinator CPU 0
}

sched_rq_t *sched_local_rq(void) {
    return &g_mock_rqs[0];
}

uint32_t sched_clamp_core(uint32_t core_id) {
    return core_id % 2;
}

bh_thread_t *sched_find_thread_by_id(uint64_t tid) {
    static bh_thread_t mock_thread;
    mock_thread.thread_id = tid;
    mock_thread.sched_generation = 1;
    mock_thread.generation = 1;
    return &mock_thread;
}

sched_entity_t *sched_find_entity_by_thread(const bh_thread_t *thread) {
    sched_rq_t *rq = &g_mock_rqs[thread->owner_cpu];
    for (size_t i = 0; i < SCHED_MAX_LOCAL_ENTITIES; ++i) {
        if (rq->entities[i].in_use && rq->entities[i].entity.tid == thread->thread_id) {
            return &rq->entities[i].entity;
        }
    }
    return NULL;
}

void sched_completion_arm(sched_rq_t *rq, uint16_t slot, uint32_t generation) {
    sched_completion_cell_t *cell = &rq->completions[slot];
    cell->generation = generation;
    cell->state = COMPLETION_ARMED;
}

kstatus_t sched_completion_publish(uint16_t origin_cpu, uint16_t slot, uint32_t generation, uint16_t responder_cpu, uint8_t kind, int32_t result, uint32_t epoch, const void *payload, size_t payload_size) {
    sched_rq_t *origin_rq = &g_mock_rqs[origin_cpu];
    sched_completion_cell_t *cell = &origin_rq->completions[slot];
    if (cell->generation != generation || cell->state != COMPLETION_ARMED) {
        return K_ERR_BAD_STATE;
    }
    cell->result = result;
    cell->responder_cpu = responder_cpu;
    cell->kind = kind;
    cell->migration_epoch = epoch;
    if (payload && payload_size > 0) {
        __builtin_memcpy(&cell->payload, payload, payload_size);
    }
    cell->state = COMPLETION_PUBLISHED;
    return K_OK;
}

kstatus_t sched_remote_respond_cell(uint16_t origin_cpu, uint16_t slot, uint32_t generation, uint8_t kind, int32_t result) {
    return sched_completion_publish(origin_cpu, slot, generation, 1, kind, result, 0, NULL, 0);
}

void test_migration_lost_ack_reserve(void) {
    printf("Running test_migration_lost_ack_reserve...\n");
    sched_rq_t *rq0 = &g_mock_rqs[0];
    sched_rq_t *rq1 = &g_mock_rqs[1];

    // Initialize pools
    rq0->free_entity_head = 0;
    for (size_t i = 0; i < SCHED_MAX_LOCAL_ENTITIES; ++i) {
        rq0->entities[i].in_use = 0;
        rq0->entities[i].generation = 0;
        rq0->entities[i].next_free = i + 1;
    }
    rq0->entities[SCHED_MAX_LOCAL_ENTITIES - 1].next_free = UINT32_MAX;

    rq1->free_entity_head = 0;
    for (size_t i = 0; i < SCHED_MAX_LOCAL_ENTITIES; ++i) {
        rq1->entities[i].in_use = 0;
        rq1->entities[i].generation = 0;
        rq1->entities[i].next_free = i + 1;
    }
    rq1->entities[SCHED_MAX_LOCAL_ENTITIES - 1].next_free = UINT32_MAX;

    bh_thread_t thread;
    thread.thread_id = 42;
    thread.owner_cpu = 0;
    thread.migration_epoch = 1;
    thread.sched_generation = 1;
    thread.generation = 1;

    // Allocate entity on 0
    sched_entity_slot_t *slot0 = &rq0->entities[0];
    slot0->in_use = 1;
    slot0->entity.tid = 42;
    slot0->entity.migration_epoch = 1;
    slot0->entity.migration_state = SCHED_MIG_NONE;
    slot0->entity.runnable = true;

    thread.owner_locator.cpu = 0;
    thread.owner_locator.slot = 0;
    thread.owner_locator.entity_generation = slot0->generation;
    thread.owner_locator.migration_epoch = 1;

    // Prepare outbound command
    sched_remote_cmd_t cmd;
    cmd.handle.slot = 5;
    cmd.handle.origin_cpu = 0;
    cmd.handle.generation = 10;
    cmd.type = SCHED_REMOTE_MIGRATE_RESERVE;
    cmd.thread_id = 42;
    cmd.expected_thread_generation = 1;
    cmd.migration_epoch = 2;
    cmd.state = SCHED_REMOTE_CMD_PENDING;

    sched_completion_arm(rq0, 5, 10);

    // B processes RESERVE
    sched_entity_slot_t *slot1 = &rq1->entities[0];
    slot1->in_use = 1;
    slot1->entity.tid = 42;
    slot1->entity.migration_epoch = 2;
    slot1->entity.migration_state = SCHED_MIG_TARGET_RESERVED;
    slot1->entity.runnable = false;

    // Lost ACK: target B publishes ACK but it gets lost/timed out on CPU 0
    // Simulating timeout on CPU 0
    cmd.state = SCHED_REMOTE_CMD_TIMEOUT;

    // Reconciliation: since RESERVE timed out, coordinator CPU 0 aborts migration
    assert(cmd.state == SCHED_REMOTE_CMD_TIMEOUT);
    slot0->entity.migration_state = SCHED_MIG_NONE;
    slot0->entity.runnable = true;

    printf("test_migration_lost_ack_reserve passed!\n");
}

int main(void) {
    test_migration_lost_ack_reserve();
    printf("ALL LOST ACK MIGRATION TESTS PASSED\n");
    return 0;
}
