#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CPUS 32U
#define SCHED_REMOTE_CMD_CAPACITY 256U
#define SCHED_CMD_BITMAP_WORDS 8

typedef enum {
    SCHED_REMOTE_CMD_EMPTY = 0,
    SCHED_REMOTE_CMD_RESERVED,
    SCHED_REMOTE_CMD_PENDING,
    SCHED_REMOTE_CMD_ACKED,
    SCHED_REMOTE_CMD_FAILED,
    SCHED_REMOTE_CMD_TIMEOUT
} sched_remote_cmd_state_t;

typedef struct {
    uint16_t slot;
    uint16_t origin_cpu;
    uint32_t generation;
} sched_cmd_handle_t;

typedef struct {
    struct {
        void *next, *prev;
    } list;
} list_head_t;

typedef struct sched_remote_cmd {
    sched_cmd_handle_t handle;
    uint64_t cmd_id;
    uint32_t type;
    volatile uint32_t state;
    uint32_t source_cpu;
    uint32_t target_cpu;
    uint64_t thread_id;
    uint64_t expected_thread_generation;
    uint32_t migration_epoch;
    int32_t result;
    uint64_t submit_tick;
    uint64_t deadline_tick;
    uint32_t flags;
    uint32_t priority;
    list_head_t list;
} sched_remote_cmd_t;

typedef struct {
    sched_remote_cmd_t outbound_cmds[SCHED_REMOTE_CMD_CAPACITY];
    volatile uint32_t outbound_alloc_bitmap[SCHED_CMD_BITMAP_WORDS];
    uint64_t total_ticks;
} sched_rq_t;

typedef struct {
    sched_rq_t runqueue;
} cpu_local_t;

cpu_local_t g_cpu_locals[MAX_CPUS];
uint32_t g_active_core_count = 4;
static uint32_t mock_cpu_id = 0;

uint32_t hal_cpu_get_id(void) { return mock_cpu_id; }
uint32_t sched_clamp_core(uint32_t core_id) { return core_id; }

sched_remote_cmd_t *test_allocate_outbound_cmd(void) {
    uint32_t core = sched_clamp_core(hal_cpu_get_id());
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;
    uint32_t slot_idx = 0xFFFF;

    for (uint32_t w = 0; w < SCHED_CMD_BITMAP_WORDS; ++w) {
        uint32_t val = __atomic_load_n(&rq->outbound_alloc_bitmap[w], __ATOMIC_ACQUIRE);
        while (val != 0xFFFFFFFFU) {
            uint32_t free_bit = __builtin_ctz(~val);
            uint32_t new_val = val | (1U << free_bit);
            if (__atomic_compare_exchange_n(&rq->outbound_alloc_bitmap[w], &val, new_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
                slot_idx = w * 32 + free_bit;
                break;
            }
        }
        if (slot_idx != 0xFFFF) {
            break;
        }
    }

    if (slot_idx >= SCHED_REMOTE_CMD_CAPACITY) {
        return NULL;
    }

    sched_remote_cmd_t *cmd = &rq->outbound_cmds[slot_idx];
    cmd->handle.generation++;
    if (cmd->handle.generation == 0) {
        cmd->handle.generation = 1;
    }
    cmd->cmd_id = slot_idx;
    cmd->state = SCHED_REMOTE_CMD_RESERVED;
    return cmd;
}

void test_remote_cmd_release(sched_remote_cmd_t *cmd) {
    if (!cmd) return;
    uint16_t slot = cmd->handle.slot;
    uint32_t w = slot / 32;
    uint32_t bit = slot % 32;
    uint32_t mask = ~(1U << bit);

    __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_EMPTY, __ATOMIC_RELEASE);

    uint32_t core = cmd->handle.origin_cpu;
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;
    __atomic_fetch_and(&rq->outbound_alloc_bitmap[w], mask, __ATOMIC_ACQ_REL);
}

void test_alloc_and_exhaustion(void) {
    printf("Running test_alloc_and_exhaustion...\n");
    mock_cpu_id = 0;
    sched_rq_t *rq = &g_cpu_locals[0].runqueue;
    memset(rq, 0, sizeof(sched_rq_t));
    for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
        rq->outbound_cmds[i].handle.slot = i;
        rq->outbound_cmds[i].handle.origin_cpu = 0;
        rq->outbound_cmds[i].handle.generation = 1;
        rq->outbound_cmds[i].state = SCHED_REMOTE_CMD_EMPTY;
    }

    // Allocate all 256 slots
    sched_remote_cmd_t *cmds[SCHED_REMOTE_CMD_CAPACITY];
    for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
        cmds[i] = test_allocate_outbound_cmd();
        assert(cmds[i] != NULL);
        assert(cmds[i]->cmd_id == i);
        assert(cmds[i]->state == SCHED_REMOTE_CMD_RESERVED);
    }

    // Next allocation must fail due to exhaustion
    sched_remote_cmd_t *fail_cmd = test_allocate_outbound_cmd();
    assert(fail_cmd == NULL);

    // Release slot 5
    test_remote_cmd_release(cmds[5]);
    assert(rq->outbound_cmds[5].state == SCHED_REMOTE_CMD_EMPTY);

    // Re-allocate should get slot 5 and incremented generation
    sched_remote_cmd_t *re_cmd = test_allocate_outbound_cmd();
    assert(re_cmd != NULL);
    assert(re_cmd->cmd_id == 5);
    assert(re_cmd->handle.generation == 3); // 2 -> 3
    printf("test_alloc_and_exhaustion passed!\n");
}

int main(void) {
    test_alloc_and_exhaustion();
    printf("ALL TESTS PASSED\n");
    return 0;
}
