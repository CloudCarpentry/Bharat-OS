#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

typedef enum {
    SCHED_REMOTE_WAKE,
    SCHED_REMOTE_MIGRATE,
    SCHED_REMOTE_BLOCK,
    SCHED_REMOTE_YIELD,
    SCHED_REMOTE_ENQUEUE,
    SCHED_REMOTE_DEQUEUE,
    SCHED_REMOTE_HANDOFF,
    SCHED_REMOTE_SET_AFFINITY,
    SCHED_REMOTE_QUARANTINE,
    SCHED_REMOTE_STEAL_REQ,
    SCHED_REMOTE_SET_PRIORITY,
    SCHED_REMOTE_TERMINATE,
    SCHED_REMOTE_REAP
} sched_remote_cmd_type_t;

typedef struct {
    uint16_t slot;
    uint16_t origin_cpu;
    uint32_t generation;
} sched_cmd_handle_t;

typedef struct {
    sched_cmd_handle_t handle;
    sched_remote_cmd_type_t type;
    uint32_t source_cpu;
    uint32_t target_cpu;
    uint64_t thread_id;
    uint64_t expected_thread_generation;
    uint32_t migration_epoch;
    uint32_t flags;
    uint32_t priority;
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

typedef struct {
    sched_cmd_ring_t cmd_ring;
    sched_completion_ring_t completion_ring;
    uint32_t resched_pending;
} sched_remote_inbox_t;

typedef struct {
    sched_remote_cmd_envelope_t outbound_cmds[SCHED_REMOTE_CMD_CAPACITY];
    volatile uint32_t outbound_alloc_bitmap[SCHED_CMD_BITMAP_WORDS];
    sched_remote_inbox_t remote;
} sched_rq_t;

typedef struct {
    sched_rq_t runqueue;
} cpu_local_t;

cpu_local_t g_cpu_locals[MAX_CPUS];
uint32_t g_active_core_count = 4;
static uint32_t mock_cpu_id = 0;

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
    sched_cmd_slot_t *slot;
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

bool sched_cmd_ring_pop(sched_cmd_ring_t *q, sched_remote_cmd_envelope_t *out_value) {
    sched_cmd_slot_t *slot;
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

void test_basic_publish_consume(void) {
    printf("Running test_basic_publish_consume...\n");
    sched_cmd_ring_t ring;
    sched_cmd_ring_init(&ring, 4);

    sched_remote_cmd_envelope_t env = {
        .handle = { .slot = 1, .origin_cpu = 0, .generation = 10 },
        .type = SCHED_REMOTE_WAKE,
        .thread_id = 100
    };

    bool ok = sched_cmd_ring_push(&ring, &env);
    assert(ok);

    sched_remote_cmd_envelope_t popped;
    ok = sched_cmd_ring_pop(&ring, &popped);
    assert(ok);
    assert(popped.handle.slot == 1);
    assert(popped.handle.generation == 10);
    assert(popped.thread_id == 100);

    // Empty now
    ok = sched_cmd_ring_pop(&ring, &popped);
    assert(!ok);
    printf("test_basic_publish_consume passed!\n");
}

void test_queue_full(void) {
    printf("Running test_queue_full...\n");
    sched_cmd_ring_t ring;
    sched_cmd_ring_init(&ring, 2);

    sched_remote_cmd_envelope_t env = { .thread_id = 1 };
    assert(sched_cmd_ring_push(&ring, &env));
    assert(sched_cmd_ring_push(&ring, &env));
    assert(!sched_cmd_ring_push(&ring, &env)); // Full

    sched_remote_cmd_envelope_t popped;
    assert(sched_cmd_ring_pop(&ring, &popped));
    assert(sched_cmd_ring_push(&ring, &env)); // Can push again
    printf("test_queue_full passed!\n");
}

int main(void) {
    test_basic_publish_consume();
    test_queue_full();
    printf("ALL BASIC PROTOCOL TESTS PASSED\n");
    return 0;
}
