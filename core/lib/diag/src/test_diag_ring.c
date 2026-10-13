#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "bharat/diag/diag_ring.h"
#include <stdlib.h>

#define CAPACITY 64
#define PAYLOAD_MAX 32
#define THREADS 4
#define ITERS 1000

bh_diag_ring_t ring;
bh_diag_ring_slot_t slots[CAPACITY];

void *producer_thread(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERS; i++) {
        bh_diag_event_header_t header = {0};
        header.abi_version = BH_DIAG_ABI_VERSION;
        header.header_size = sizeof(header);
        header.payload_size = 0;
        header.severity = 0;
        header.source_kind = 0;

        // Spin if full
        while (bh_diag_ring_try_write(&ring, &header, NULL) == BH_ERR_BUFFER_FULL) {
        }
    }
    return NULL;
}

int main() {
    bh_status_t status = bh_diag_ring_init(&ring, slots, CAPACITY, PAYLOAD_MAX);
    assert(status == BH_OK);

    pthread_t threads[THREADS];
    for (int i = 0; i < THREADS; i++) {
        pthread_create(&threads[i], NULL, producer_thread, NULL);
    }

    int read = 0;
    while (read < THREADS * ITERS) {
        bh_diag_record_t record;
        status = bh_diag_ring_try_read(&ring, &record);
        if (status == BH_OK) {
            read++;
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    bh_diag_ring_stats_t stats;
    bh_diag_ring_get_stats(&ring, &stats);

    assert(stats.accepted == THREADS * ITERS);
    assert(stats.consumed == THREADS * ITERS);
    assert(stats.corrupt == 0);

    printf("Diag ring stress test completed.\n");
    return 0;
}
