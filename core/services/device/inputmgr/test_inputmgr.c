#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "inputmgr/inputmgr.h"

// Define stub structures and enums if not available directly for isolated testing
// Since we have inputmgr.h we can just include it.

void *producer_thread(void *arg) {
    for (int i = 0; i < 1000; i++) {
        bh_inputmgr_enqueue(1, 2, 0, 1);
    }
    return NULL;
}

void *consumer_thread(void *arg) {
    int total_drained = 0;
    int total_value = 0;
    bh_input_event_t evs[10];

    // We try to drain events. Coalescing might happen.
    for (int i = 0; i < 1000; i++) {
        int count = bh_inputmgr_drain(evs, 10);
        for(int j=0; j<count; j++) {
            total_drained++;
            total_value += evs[j].value;
        }
    }
    return NULL;
}

int main() {
    inputmgr_init();

    pthread_t p, c;
    pthread_create(&p, NULL, producer_thread, NULL);
    pthread_create(&c, NULL, consumer_thread, NULL);

    pthread_join(p, NULL);
    pthread_join(c, NULL);

    // Drain remaining
    bh_input_event_t evs[10];
    int count;
    int total_value = 0;
    while ((count = bh_inputmgr_drain(evs, 10)) > 0) {
        for(int j=0; j<count; j++) {
            total_value += evs[j].value;
        }
    }

    printf("Test completed\n");
    return 0;
}
