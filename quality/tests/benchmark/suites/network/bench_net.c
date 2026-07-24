#include "bench.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t payload[1500];
    uint32_t len;
} netbuf_t;

static int net_setup(void **ctx) {
    // Mock netbuf pool
    netbuf_t *pool = malloc(sizeof(netbuf_t) * 1024);
    *ctx = pool;
    return pool ? 0 : -1;
}

static int net_run_loopback(void *ctx, uint64_t iterations) {
    // Mock UDP loopback send/recv cost
    netbuf_t *pool = ctx;
    uint32_t idx = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        // Enqueue / Dequeue / Copy
        netbuf_t *pkt = &pool[idx];
        pkt->len = 64;
        memset(pkt->payload, 0xFF, pkt->len);
        idx = (idx + 1) % 1024;
    }
    return 0;
}

static void net_teardown(void *ctx) {
    free(ctx);
}

REGISTER_BENCHMARK(net_loopback_64b, "network", BENCH_LEVEL_B1, 1000000, 100000, net_setup, net_run_loopback, net_teardown);
