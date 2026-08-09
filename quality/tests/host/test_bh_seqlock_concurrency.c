#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <bharat/kernel/ds/bh_seqlock.h>

#define NUM_READERS 4
#define NUM_WRITERS 2
#define ITERATIONS 100000

// Constant to XOR against
#define MAGIC_CONSTANT 0xDEADBEEF

struct snapshot_data {
    uint64_t generation;
    uint64_t inverse;
    uint64_t checksum;
    uint32_t a;
    uint32_t b;
};

struct shared_state {
    bh_seqlock_t lock;
    struct snapshot_data data;
    atomic32_t global_writer_gen;
};

struct shared_state state;

void* reader_thread(void* arg) {
    (void)arg;
    struct snapshot_data copy;

    for (int i = 0; i < ITERATIONS; i++) {
        uint32_t seq;
        do {
            seq = bh_seqlock_read_begin(&state.lock);

            // Read snapshot
            copy.generation = state.data.generation;
            copy.inverse = state.data.inverse;
            copy.checksum = state.data.checksum;
            copy.a = state.data.a;
            copy.b = state.data.b;

        } while (bh_seqlock_read_retry(&state.lock, seq));

        // Assert invariants (only if generation > 0 as it starts at 0 without invariants initially)
        if (copy.generation > 0) {
            assert(copy.inverse == ~copy.generation);
            assert(copy.a == (uint32_t)(copy.generation & 0xFFFFFFFF));
            assert(copy.b == (copy.a ^ MAGIC_CONSTANT));
            uint64_t expected_checksum = copy.generation ^ copy.inverse ^ copy.a ^ copy.b;
            assert(copy.checksum == expected_checksum);
        }
    }
    return NULL;
}

void* writer_thread(void* arg) {
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        uint32_t gen = atomic32_fetch_add_relaxed(&state.global_writer_gen, 1) + 1;

        bh_seqlock_write_begin(&state.lock);

        // Update snapshot
        state.data.generation = gen;
        state.data.inverse = ~((uint64_t)gen);
        state.data.a = (uint32_t)(gen & 0xFFFFFFFF);
        state.data.b = state.data.a ^ MAGIC_CONSTANT;
        state.data.checksum = state.data.generation ^ state.data.inverse ^ state.data.a ^ state.data.b;

        bh_seqlock_write_end(&state.lock);
    }
    return NULL;
}

int main() {
    bh_seqlock_init(&state.lock);
    atomic32_set(&state.global_writer_gen, 0);

    // Initialize data cleanly
    state.data.generation = 0;

    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_create(&readers[i], NULL, reader_thread, NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_create(&writers[i], NULL, writer_thread, NULL);
    }

    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }

    printf("Concurrency test passed.\n");
    return 0;
}
