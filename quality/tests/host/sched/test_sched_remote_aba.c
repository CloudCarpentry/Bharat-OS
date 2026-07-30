#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SCHED_REMOTE_CMD_CAPACITY 256U

typedef struct {
    uint16_t slot;
    uint16_t origin_cpu;
    uint32_t generation;
} sched_cmd_handle_t;

typedef enum {
    SCHED_REMOTE_CMD_EMPTY = 0,
    SCHED_REMOTE_CMD_PENDING,
    SCHED_REMOTE_CMD_ACKED,
    SCHED_REMOTE_CMD_FAILED,
    SCHED_REMOTE_CMD_TIMEOUT
} sched_remote_cmd_state_t;

typedef struct {
    sched_cmd_handle_t handle;
    uint32_t state;
    uint64_t thread_id;
} sched_remote_cmd_t;

typedef struct {
    sched_cmd_handle_t handle;
    uint64_t thread_id;
} sched_remote_cmd_envelope_t;

typedef struct {
    volatile uint64_t seq;
    sched_remote_cmd_envelope_t value;
} sched_cmd_slot_t;

typedef struct {
    sched_cmd_slot_t slots[SCHED_REMOTE_CMD_CAPACITY];
    uint32_t capacity;
    uint32_t mask;
    volatile uint64_t head;
    uint64_t tail;
} sched_cmd_ring_t;

typedef enum {
    SCHED_COMPLETION_ACK = 0,
    SCHED_COMPLETION_NACK,
} sched_completion_kind_t;

typedef struct {
    sched_cmd_handle_t handle;
    int32_t result;
    uint8_t kind;
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

void sched_cmd_ring_init(sched_cmd_ring_t *q, uint32_t capacity) {
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;
    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
    }
}

bool sched_cmd_ring_push(sched_cmd_ring_t *q, const sched_remote_cmd_envelope_t *value) {
    sched_cmd_slot_t *slot = &q->slots[q->head & q->mask];
    q->head++;
    slot->value = *value;
    return true;
}

bool sched_cmd_ring_pop(sched_cmd_ring_t *q, sched_remote_cmd_envelope_t *out_value) {
    if (q->tail == q->head) return false;
    sched_cmd_slot_t *slot = &q->slots[q->tail & q->mask];
    q->tail++;
    *out_value = slot->value;
    return true;
}

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
    sched_completion_slot_t *slot = &q->slots[q->head & q->mask];
    q->head++;
    slot->value = *value;
    return true;
}

bool sched_completion_ring_pop(sched_completion_ring_t *q, sched_remote_completion_t *out_value) {
    if (q->tail == q->head) return false;
    sched_completion_slot_t *slot = &q->slots[q->tail & q->mask];
    q->tail++;
    *out_value = slot->value;
    return true;
}

void test_remote_aba_safe(void) {
    printf("Running test_remote_aba_safe...\n");
    sched_cmd_ring_t cmd_ring;
    sched_cmd_ring_init(&cmd_ring, 256);

    sched_completion_ring_t comp_ring;
    sched_completion_ring_init(&comp_ring, 256);

    sched_remote_cmd_t outbound_cmds[SCHED_REMOTE_CMD_CAPACITY];
    memset(outbound_cmds, 0, sizeof(outbound_cmds));

    // 1. Allocate slot 7 gen 41
    outbound_cmds[7].handle.slot = 7;
    outbound_cmds[7].handle.origin_cpu = 0;
    outbound_cmds[7].handle.generation = 41;
    outbound_cmds[7].state = SCHED_REMOTE_CMD_PENDING;
    outbound_cmds[7].thread_id = 101; // Command A

    // Submit A to cmd ring
    sched_remote_cmd_envelope_t envA = {
        .handle = outbound_cmds[7].handle,
        .thread_id = outbound_cmds[7].thread_id
    };
    assert(sched_cmd_ring_push(&cmd_ring, &envA));

    // 2. Timeout A on source side
    outbound_cmds[7].state = SCHED_REMOTE_CMD_TIMEOUT;

    // 3. Release slot 7 and reuse with gen 42 (Command B)
    outbound_cmds[7].handle.generation = 42;
    outbound_cmds[7].state = SCHED_REMOTE_CMD_PENDING;
    outbound_cmds[7].thread_id = 202; // Command B

    sched_remote_cmd_envelope_t envB = {
        .handle = outbound_cmds[7].handle,
        .thread_id = outbound_cmds[7].thread_id
    };
    assert(sched_cmd_ring_push(&cmd_ring, &envB));

    // 4. Consume delayed A (envelope carries immutable handle and thread_id)
    sched_remote_cmd_envelope_t poppedA;
    assert(sched_cmd_ring_pop(&cmd_ring, &poppedA));
    assert(poppedA.handle.generation == 41);
    assert(poppedA.thread_id == 101); // Consumer sees gen 41, cannot observe command B data!

    // Respond to A
    sched_remote_completion_t compA = {
        .handle = poppedA.handle,
        .kind = SCHED_COMPLETION_ACK,
        .result = 0
    };
    assert(sched_completion_ring_push(&comp_ring, &compA));

    // 5. Source CPU polls completion ring
    sched_remote_completion_t popped_comp;
    assert(sched_completion_ring_pop(&comp_ring, &popped_comp));

    // Generation check on outbound_cmds[7]
    sched_remote_cmd_t *cmd7 = &outbound_cmds[popped_comp.handle.slot];
    if (cmd7->handle.generation != popped_comp.handle.generation) {
        // Correctly detected and discarded stale completion of command A!
        // Command B state remains PENDING!
        assert(cmd7->handle.generation == 42);
        assert(cmd7->state == SCHED_REMOTE_CMD_PENDING);
    } else {
        assert(false && "Failed: Stale completion mutated the new generation command!");
    }

    // 6. Consume B
    sched_remote_cmd_envelope_t poppedB;
    assert(sched_cmd_ring_pop(&cmd_ring, &poppedB));
    assert(poppedB.handle.generation == 42);
    assert(poppedB.thread_id == 202);

    sched_remote_completion_t compB = {
        .handle = poppedB.handle,
        .kind = SCHED_COMPLETION_ACK,
        .result = 0
    };
    assert(sched_completion_ring_push(&comp_ring, &compB));

    assert(sched_completion_ring_pop(&comp_ring, &popped_comp));
    assert(cmd7->handle.generation == popped_comp.handle.generation);
    cmd7->state = (popped_comp.kind == SCHED_COMPLETION_ACK) ? SCHED_REMOTE_CMD_ACKED : SCHED_REMOTE_CMD_FAILED;
    assert(cmd7->state == SCHED_REMOTE_CMD_ACKED);

    printf("test_remote_aba_safe passed!\n");
}

int main(void) {
    test_remote_aba_safe();
    printf("ALL ABA-SAFETY TESTS PASSED\n");
    return 0;
}
