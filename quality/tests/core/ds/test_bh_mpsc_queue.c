#include <bharat/kernel/ds/bh_mpsc_queue.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TEST_CAPACITY 8

void test_mpsc_basic() {
    bh_mpsc_queue_t q;
    bh_mpsc_slot_t slots[TEST_CAPACITY];

    assert(bh_mpsc_queue_init(&q, slots, TEST_CAPACITY) == K_OK);
    assert(bh_mpsc_queue_empty(&q));
    assert(bh_mpsc_queue_capacity(&q) == TEST_CAPACITY);

    void *val;
    assert(bh_mpsc_queue_pop(&q, &val) == K_ERR_AGAIN);

    int data1 = 123;
    assert(bh_mpsc_queue_push(&q, &data1) == K_OK);
    assert(!bh_mpsc_queue_empty(&q));

    assert(bh_mpsc_queue_pop(&q, &val) == K_OK);
    assert(val == &data1);
    assert(bh_mpsc_queue_empty(&q));

    printf("test_mpsc_basic passed\n");
}

void test_mpsc_full() {
    bh_mpsc_queue_t q;
    bh_mpsc_slot_t slots[TEST_CAPACITY];
    int data[TEST_CAPACITY];

    bh_mpsc_queue_init(&q, slots, TEST_CAPACITY);

    for (int i = 0; i < TEST_CAPACITY; i++) {
        data[i] = i;
        assert(bh_mpsc_queue_push(&q, &data[i]) == K_OK);
    }

    assert(bh_mpsc_queue_push(&q, &data[0]) == K_ERR_AGAIN);

    void *val;
    for (int i = 0; i < TEST_CAPACITY; i++) {
        assert(bh_mpsc_queue_pop(&q, &val) == K_OK);
        assert(*(int *)val == i);
    }
    assert(bh_mpsc_queue_pop(&q, &val) == K_ERR_AGAIN);

    printf("test_mpsc_full passed\n");
}

#define THREADED_COUNT 100000
#define NUM_PRODUCERS 4

typedef struct {
    bh_mpsc_queue_t *q;
    int id;
} producer_args_t;

void *producer_fn(void *arg) {
    producer_args_t *args = (producer_args_t *)arg;
    for (int i = 0; i < THREADED_COUNT; i++) {
        while (bh_mpsc_queue_push(args->q, (void *)(intptr_t)((args->id << 20) | i)) == K_ERR_AGAIN) {
            // spin
        }
    }
    return NULL;
}

void test_mpsc_threaded() {
    bh_mpsc_queue_t q;
    bh_mpsc_slot_t *slots = malloc(sizeof(bh_mpsc_slot_t) * 1024);
    bh_mpsc_queue_init(&q, slots, 1024);

    pthread_t producers[NUM_PRODUCERS];
    producer_args_t args[NUM_PRODUCERS];

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        args[i].q = &q;
        args[i].id = i + 1;
        pthread_create(&producers[i], NULL, producer_fn, &args[i]);
    }

    int received_counts[NUM_PRODUCERS + 1] = {0};
    int total_to_receive = NUM_PRODUCERS * THREADED_COUNT;
    int received = 0;

    while (received < total_to_receive) {
        void *val;
        if (bh_mpsc_queue_pop(&q, &val) == K_OK) {
            intptr_t v = (intptr_t)val;
            int id = v >> 20;
            assert(id > 0 && id <= NUM_PRODUCERS);
            received_counts[id]++;
            received++;
        }
    }

    for (int i = 1; i <= NUM_PRODUCERS; i++) {
        assert(received_counts[i] == THREADED_COUNT);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    free(slots);
    printf("test_mpsc_threaded passed\n");
}

void test_mpsc_wraparound() {
    bh_mpsc_queue_t q;
    bh_mpsc_slot_t slots[TEST_CAPACITY];

    assert(bh_mpsc_queue_init(&q, slots, TEST_CAPACITY) == K_OK);

    // Manually advance state to near UINT32_MAX
    uint32_t near_max = 0xFFFFFFFC;
    atomic32_store_relaxed(&q.head, near_max);
    q.tail = near_max;
    for (int i = 0; i < TEST_CAPACITY; i++) {
        atomic32_store_relaxed(&q.slots[(near_max + i) & q.mask].seq, near_max + i);
    }

    int data[TEST_CAPACITY];
    for (int i = 0; i < TEST_CAPACITY; i++) {
        data[i] = i;
    }

    // Push until full around wrap boundary
    for (int i = 0; i < TEST_CAPACITY; i++) {
        assert(bh_mpsc_queue_push(&q, &data[i]) == K_OK);
    }

    // Full condition
    assert(bh_mpsc_queue_push(&q, &data[0]) == K_ERR_AGAIN);

    // Pop across wrap
    void *val;
    for (int i = 0; i < TEST_CAPACITY; i++) {
        assert(bh_mpsc_queue_pop(&q, &val) == K_OK);
        assert(*(int *)val == i);
    }

    // Empty condition
    assert(bh_mpsc_queue_pop(&q, &val) == K_ERR_AGAIN);

    // Reuse slot after wrap
    assert(bh_mpsc_queue_push(&q, &data[0]) == K_OK);
    assert(bh_mpsc_queue_pop(&q, &val) == K_OK);
    assert(*(int *)val == 0);

    printf("test_mpsc_wraparound passed\n");
}

#define RACE_CAPACITY 4
#define RACE_PRODUCERS 8
#define RACE_COUNT 10000

void *race_producer_fn(void *arg) {
    producer_args_t *args = (producer_args_t *)arg;
    for (int i = 0; i < RACE_COUNT; i++) {
        while (bh_mpsc_queue_push(args->q, (void *)(intptr_t)((args->id << 20) | i)) == K_ERR_AGAIN) {
            // spin
        }
    }
    return NULL;
}

void test_mpsc_producer_race() {
    bh_mpsc_queue_t q;
    bh_mpsc_slot_t slots[RACE_CAPACITY];
    bh_mpsc_queue_init(&q, slots, RACE_CAPACITY);

    pthread_t producers[RACE_PRODUCERS];
    producer_args_t args[RACE_PRODUCERS];

    for (int i = 0; i < RACE_PRODUCERS; i++) {
        args[i].q = &q;
        args[i].id = i + 1;
        pthread_create(&producers[i], NULL, race_producer_fn, &args[i]);
    }

    int received_counts[RACE_PRODUCERS + 1] = {0};
    int total_to_receive = RACE_PRODUCERS * RACE_COUNT;
    int received = 0;

    while (received < total_to_receive) {
        void *val;
        if (bh_mpsc_queue_pop(&q, &val) == K_OK) {
            intptr_t v = (intptr_t)val;
            int id = v >> 20;
            assert(id > 0 && id <= RACE_PRODUCERS);
            received_counts[id]++;
            received++;
        }
    }

    for (int i = 1; i <= RACE_PRODUCERS; i++) {
        assert(received_counts[i] == RACE_COUNT);
    }

    for (int i = 0; i < RACE_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    printf("test_mpsc_producer_race passed\n");
}

void test_mpsc_null_args() {
    bh_mpsc_queue_t q;
    bh_mpsc_slot_t slots[TEST_CAPACITY];

    assert(bh_mpsc_queue_init(NULL, slots, TEST_CAPACITY) == K_ERR_INVALID_ARG);
    assert(bh_mpsc_queue_init(&q, NULL, TEST_CAPACITY) == K_ERR_INVALID_ARG);

    assert(bh_mpsc_queue_init(&q, slots, TEST_CAPACITY) == K_OK);

    int val = 42;
    assert(bh_mpsc_queue_push(NULL, &val) == K_ERR_INVALID_ARG);

    void *out_val;
    assert(bh_mpsc_queue_pop(NULL, &out_val) == K_ERR_INVALID_ARG);

    printf("test_mpsc_null_args passed\n");
}


int main() {
    test_mpsc_basic();
    test_mpsc_full();
    test_mpsc_threaded();
    test_mpsc_wraparound();
    test_mpsc_producer_race();
    test_mpsc_null_args();
    return 0;
}
