#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "lib/ds/urpc_ring.h"

static urpc_ring_t test_ring;
static volatile int stop = 0;

void* producer(void* arg) {
    char msg[URPC_MSG_SIZE];
    memset(msg, 0xAA, URPC_MSG_SIZE);
    while (!stop) {
        urpc_ring_send(&test_ring, msg);
    }
    return NULL;
}

void* consumer(void* arg) {
    char msg[URPC_MSG_SIZE];
    while (!stop) {
        if (urpc_ring_recv(&test_ring, msg) == 0) {
            // Check if msg is fully written
            for (int i = 0; i < URPC_MSG_SIZE; i++) {
                if (msg[i] != (char)0xAA) {
                    printf("Defect demonstrated: Read partially written message! msg[%d] = 0x%x\n", i, msg[i] & 0xFF);
                    stop = 1;
                    return (void*)1;
                }
            }
            // Clear message to catch reuse without write
            // Wait, we can't just memset the buffer array because the producer is still producing!
            // Doing memset here races with the producer and creates the defect artificially!
        }
    }
    return NULL;
}

int main() {
    urpc_ring_init(&test_ring);
    pthread_t p, c;
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);

    int defect_found = 0;
    void *ret;
    // loop until defect found or 2 seconds passed
    for(int i=0; i<200; i++) {
        if (stop) {
            defect_found = 1;
            break;
        }
        struct timespec ts = {0, 10000000}; // 10ms
        nanosleep(&ts, NULL);
    }
    stop = 1;
    pthread_join(c, &ret);
    pthread_join(p, NULL);

    if (ret != NULL) {
        printf("Defect confirmed.\n");
        return 1;
    }
    printf("No defect observed (timing dependent).\n");
    return 0;
}
