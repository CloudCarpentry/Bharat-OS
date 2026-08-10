#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "ipc/mk_proto.h"
#include "fake_hal.h"

#define NUM_PRODUCERS 4
#define MSG_PER_PRODUCER 25000

static bh_mk_endpoint_handle_t g_ep;
static _Atomic uint32_t g_consumer_received = 0;
static _Atomic uint32_t g_producer_sent = 0;

static kstatus_t stress_handler(
    bh_mk_endpoint_handle_t source_endpoint,
    const bh_mk_wire_message_t *message,
    void *ctx)
{
    (void)source_endpoint; (void)message; (void)ctx;
    atomic_fetch_add_explicit(&g_consumer_received, 1, memory_order_relaxed);
    return K_OK;
}

static void *producer_fn(void *arg) {
    uintptr_t pid = (uintptr_t)arg;
    fake_hal_set_cpu_id((uint32_t)pid);

    for (int i = 0; i < MSG_PER_PRODUCER; i++) {
        kstatus_t st;
        do {
            st = bh_mk_send(BH_MK_ENDPOINT_LEGACY, g_ep, 1, 1, BH_MK_LANE_NORMAL, NULL, 0, NULL);
            if (st == K_ERR_WOULD_BLOCK) {
                usleep(1);
            }
        } while (st == K_ERR_WOULD_BLOCK);
        assert(st == K_OK);
        atomic_fetch_add_explicit(&g_producer_sent, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(void) {
    printf("Running test_mk_fabric_stress...\n");

    kstatus_t st = bh_mk_fabric_init(8);
    assert(st == K_OK);

    fake_hal_set_cpu_id(0);
    bh_mk_endpoint_config_t config = {
        .handler_fn = stress_handler,
        .message_class = 1,
        .opcode = 1
    };
    st = bh_mk_endpoint_bind(&config, &g_ep);
    assert(st == K_OK);

    pthread_t producers[NUM_PRODUCERS];
    for (uintptr_t i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&producers[i], NULL, producer_fn, (void*)(i + 1));
    }

    uint32_t expected_total = NUM_PRODUCERS * MSG_PER_PRODUCER;
    fake_hal_set_cpu_id(0);

    while (atomic_load_explicit(&g_consumer_received, memory_order_relaxed) < expected_total) {
        bh_mk_drain_local(1000);
        usleep(10);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    assert(atomic_load_explicit(&g_consumer_received, memory_order_relaxed) == expected_total);
    assert(atomic_load_explicit(&g_producer_sent, memory_order_relaxed) == expected_total);

    printf("test_mk_fabric_stress PASSED with %d messages!\n", expected_total);
    return 0;
}
