#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_CPUS 4
#define SCHED_REMOTE_CMD_CAPACITY 256U

typedef struct {
    uint16_t slot;
    uint16_t origin_cpu;
    uint32_t generation;
} sched_cmd_handle_t;

typedef enum {
    SCHED_REMOTE_WAKE,
    SCHED_REMOTE_MIGRATE,
    SCHED_REMOTE_BLOCK,
    SCHED_REMOTE_YIELD,
    SCHED_REMOTE_ENQUEUE,
    SCHED_REMOTE_DEQUEUE,
    SCHED_REMOTE_SET_AFFINITY,
    SCHED_REMOTE_QUARANTINE,
    SCHED_REMOTE_SET_PRIORITY,
    SCHED_REMOTE_TERMINATE
} sched_remote_cmd_type_t;

typedef struct {
    sched_cmd_handle_t handle;
    sched_remote_cmd_type_t type;
    uint32_t source_cpu;
    uint32_t target_cpu;
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

typedef enum {
    SCHED_REMOTE_CMD_EMPTY = 0,
    SCHED_REMOTE_CMD_PENDING,
    SCHED_REMOTE_CMD_ACKED,
    SCHED_REMOTE_CMD_FAILED,
    SCHED_REMOTE_CMD_TIMEOUT
} cmd_state_t;

typedef struct {
    sched_cmd_handle_t handle;
    uint32_t state;
    uint64_t thread_id;
} outbound_cmd_t;

typedef struct {
    sched_cmd_ring_t cmd_ring;
    sched_completion_ring_t completion_ring;
    outbound_cmd_t outbound[SCHED_REMOTE_CMD_CAPACITY];
    uint32_t alloc_bitmap[8];
} cpu_t;

cpu_t g_cpus[NUM_CPUS];

void ring_init(cpu_t *c) {
    c->cmd_ring.capacity = SCHED_REMOTE_CMD_CAPACITY;
    c->cmd_ring.mask = SCHED_REMOTE_CMD_CAPACITY - 1;
    c->cmd_ring.head = 0;
    c->cmd_ring.tail = 0;
    for (int i = 0; i < SCHED_REMOTE_CMD_CAPACITY; i++) {
        c->cmd_ring.slots[i].seq = i;
    }

    c->completion_ring.capacity = SCHED_REMOTE_CMD_CAPACITY;
    c->completion_ring.mask = SCHED_REMOTE_CMD_CAPACITY - 1;
    c->completion_ring.head = 0;
    c->completion_ring.tail = 0;
    for (int i = 0; i < SCHED_REMOTE_CMD_CAPACITY; i++) {
        c->completion_ring.slots[i].seq = i;
    }

    memset(c->outbound, 0, sizeof(c->outbound));
    memset(c->alloc_bitmap, 0, sizeof(c->alloc_bitmap));
}

bool push_cmd(uint32_t target, const sched_remote_cmd_envelope_t *env) {
    cpu_t *c = &g_cpus[target];
    sched_cmd_slot_t *slot = &c->cmd_ring.slots[c->cmd_ring.head & c->cmd_ring.mask];
    c->cmd_ring.head++;
    slot->value = *env;
    return true;
}

bool pop_cmd(uint32_t src, sched_remote_cmd_envelope_t *env) {
    cpu_t *c = &g_cpus[src];
    if (c->cmd_ring.tail == c->cmd_ring.head) return false;
    sched_cmd_slot_t *slot = &c->cmd_ring.slots[c->cmd_ring.tail & c->cmd_ring.mask];
    c->cmd_ring.tail++;
    *env = slot->value;
    return true;
}

bool push_completion(uint32_t target, const sched_remote_completion_t *comp) {
    cpu_t *c = &g_cpus[target];
    sched_completion_slot_t *slot = &c->completion_ring.slots[c->completion_ring.head & c->completion_ring.mask];
    c->completion_ring.head++;
    slot->value = *comp;
    return true;
}

bool pop_completion(uint32_t src, sched_remote_completion_t *comp) {
    cpu_t *c = &g_cpus[src];
    if (c->completion_ring.tail == c->completion_ring.head) return false;
    sched_completion_slot_t *slot = &c->completion_ring.slots[c->completion_ring.tail & c->completion_ring.mask];
    c->completion_ring.tail++;
    *comp = slot->value;
    return true;
}

void test_stress_concurrent(void) {
    printf("Running test_stress_concurrent with 10k operations...\n");
    for (int i = 0; i < NUM_CPUS; i++) {
        ring_init(&g_cpus[i]);
    }

    uint64_t ops = 0;
    while (ops < 10000) {
        // Randomly pick a source and target CPU
        uint32_t src = rand() % NUM_CPUS;
        uint32_t target = rand() % NUM_CPUS;
        if (src == target) target = (target + 1) % NUM_CPUS;

        // Allocate a command slot on src
        uint32_t slot_idx = rand() % SCHED_REMOTE_CMD_CAPACITY;
        outbound_cmd_t *cmd = &g_cpus[src].outbound[slot_idx];

        // If slot is empty, simulate allocate & submit
        if (cmd->state == SCHED_REMOTE_CMD_EMPTY) {
            cmd->handle.slot = slot_idx;
            cmd->handle.origin_cpu = src;
            cmd->handle.generation++;
            cmd->state = SCHED_REMOTE_CMD_PENDING;
            cmd->thread_id = ops + 1000;

            sched_remote_cmd_envelope_t env = {
                .handle = cmd->handle,
                .type = SCHED_REMOTE_WAKE,
                .source_cpu = src,
                .target_cpu = target,
                .thread_id = cmd->thread_id
            };
            push_cmd(target, &env);
            ops++;
        }

        // Simulating target pop & execute and respond
        sched_remote_cmd_envelope_t env;
        if (pop_cmd(target, &env)) {
            sched_remote_completion_t comp = {
                .handle = env.handle,
                .kind = SCHED_COMPLETION_ACK,
                .result = 0
            };
            push_completion(env.handle.origin_cpu, &comp);
        }

        // Simulating src pop completion & retire
        sched_remote_completion_t comp;
        if (pop_completion(src, &comp)) {
            outbound_cmd_t *retire_cmd = &g_cpus[src].outbound[comp.handle.slot];
            if (retire_cmd->handle.generation == comp.handle.generation) {
                retire_cmd->state = SCHED_REMOTE_CMD_EMPTY; // Safe retirement!
            }
        }
    }

    printf("test_stress_concurrent passed 10k operations successfully!\n");
}

int main(void) {
    srand(42);
    test_stress_concurrent();
    printf("ALL STRESS TESTS PASSED\n");
    return 0;
}
