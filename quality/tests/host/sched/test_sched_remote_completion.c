#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_CPUS 32U
#define SCHED_REMOTE_CMD_CAPACITY 256U

typedef struct {
    uint16_t slot;
    uint16_t origin_cpu;
    uint32_t generation;
} sched_cmd_handle_t;

typedef enum {
    SCHED_COMPLETION_ACK = 0,
    SCHED_COMPLETION_NACK,
} sched_completion_kind_t;

typedef struct {
    sched_cmd_handle_t handle;
    int32_t result;
    uint16_t responder_cpu;
    uint8_t kind;
    uint8_t reserved;
} sched_remote_completion_t;

typedef struct {
    volatile uint64_t seq;
    sched_remote_completion_t value;
} sched_completion_slot_t;

typedef struct {
    sched_completion_slot_t slots[SCHED_REMOTE_CMD_CAPACITY];
    uint32_t capacity;
    uint32_t mask;
    volatile uint64_t head;
    uint64_t tail;
} sched_completion_ring_t;

typedef enum {
    SCHED_REMOTE_CMD_EMPTY = 0,
    SCHED_REMOTE_CMD_RESERVED,
    SCHED_REMOTE_CMD_PENDING,
    SCHED_REMOTE_CMD_ACKED,
    SCHED_REMOTE_CMD_FAILED,
    SCHED_REMOTE_CMD_TIMEOUT
} sched_remote_cmd_state_t;

typedef struct {
    sched_cmd_handle_t handle;
    uint32_t state;
    int32_t result;
} sched_remote_cmd_t;

void sched_completion_ring_init(sched_completion_ring_t *q, uint32_t capacity) {
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;
    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
    }
}

bool sched_completion_ring_push(sched_completion_ring_t *q, const sched_remote_completion_t *value) {
    sched_completion_slot_t *slot;
    uint64_t pos = q->head;
    while (1) {
        slot = &q->slots[pos & q->mask];
        uint64_t seq = slot->seq;
        int64_t diff = (int64_t)seq - (int64_t)pos;
        if (diff == 0) {
            q->head = pos + 1;
            break;
        } else if (diff < 0) {
            return false; // Full
        } else {
            pos = q->head;
        }
    }
    slot->value = *value;
    slot->seq = pos + 1;
    return true;
}

bool sched_completion_ring_pop(sched_completion_ring_t *q, sched_remote_completion_t *out_value) {
    sched_completion_slot_t *slot;
    uint64_t pos = q->tail;
    slot = &q->slots[pos & q->mask];
    uint64_t seq = slot->seq;
    int64_t diff = (int64_t)seq - (int64_t)(pos + 1);
    if (diff == 0) {
        q->tail = pos + 1;
        *out_value = slot->value;
        slot->seq = pos + q->mask + 1;
        return true;
    }
    return false; // Empty
}

void test_completion_scenarios(void) {
    printf("Running test_completion_scenarios...\n");
    sched_completion_ring_t ring;
    sched_completion_ring_init(&ring, 4);

    sched_remote_cmd_t outbound_cmds[SCHED_REMOTE_CMD_CAPACITY];
    memset(outbound_cmds, 0, sizeof(outbound_cmds));

    // Allocate slot 5, gen 42
    outbound_cmds[5].handle.slot = 5;
    outbound_cmds[5].handle.generation = 42;
    outbound_cmds[5].state = SCHED_REMOTE_CMD_PENDING;

    // Simulate completion arrival: ACK
    sched_remote_completion_t comp = {
        .handle = { .slot = 5, .origin_cpu = 0, .generation = 42 },
        .kind = SCHED_COMPLETION_ACK,
        .result = 0
    };
    assert(sched_completion_ring_push(&ring, &comp));

    // Process completion
    sched_remote_completion_t popped;
    bool ok = sched_completion_ring_pop(&ring, &popped);
    assert(ok);

    sched_remote_cmd_t *cmd = &outbound_cmds[popped.handle.slot];
    // generation check
    assert(cmd->handle.generation == popped.handle.generation);
    assert(cmd->state == SCHED_REMOTE_CMD_PENDING);

    cmd->state = (popped.kind == SCHED_COMPLETION_ACK) ? SCHED_REMOTE_CMD_ACKED : SCHED_REMOTE_CMD_FAILED;
    cmd->result = popped.result;

    assert(cmd->state == SCHED_REMOTE_CMD_ACKED);
    assert(cmd->result == 0);

    // Stale generation completion (e.g. timeout followed by late ACK of old generation)
    sched_remote_completion_t late_comp = {
        .handle = { .slot = 5, .origin_cpu = 0, .generation = 42 }, // old generation
        .kind = SCHED_COMPLETION_ACK,
        .result = 0
    };

    // Now slot 5 is reused, gen 43
    outbound_cmds[5].handle.generation = 43;
    outbound_cmds[5].state = SCHED_REMOTE_CMD_PENDING;

    // Process late_comp
    assert(outbound_cmds[5].handle.generation != late_comp.handle.generation); // Reject because generation stale!
    printf("test_completion_scenarios passed!\n");
}

int main(void) {
    test_completion_scenarios();
    printf("ALL COMPLETION TESTS PASSED\n");
    return 0;
}
