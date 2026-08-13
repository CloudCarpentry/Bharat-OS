#include "hal/hal_memops.h"

#include <stdint.h>
#include <stdio.h>

#define ARENA_SIZE 16480u
#define GUARD_SIZE 32u

static uint8_t actual[ARENA_SIZE];
static uint8_t expected[ARENA_SIZE];
static size_t fake_cpu;
static unsigned backend_calls;

static size_t current_cpu(void) { return fake_cpu; }
static void *marked_copy(void *d, const void *s, size_t n) {
    ++backend_calls;
    return hal_memcpy_scalar(d, s, n);
}
static void *marked_set(void *d, int c, size_t n) {
    ++backend_calls;
    return hal_memset_scalar(d, c, n);
}
static void *marked_move(void *d, const void *s, size_t n) {
    ++backend_calls;
    return hal_memmove_scalar(d, s, n);
}

static void model_move(uint8_t *dst, const uint8_t *src, size_t n) {
    const uintptr_t da = (uintptr_t)dst;
    const uintptr_t sa = (uintptr_t)src;

    if (n == 0u || da == sa) return;
    if (da < sa || da - sa >= n) {
        while (n-- != 0u) *dst++ = *src++;
    } else {
        dst += n;
        src += n;
        while (n-- != 0u) *--dst = *--src;
    }
}

static void reset(void) {
    for (size_t i = 0; i < ARENA_SIZE; ++i) {
        actual[i] = (uint8_t)((i * 37u + 11u) & 0xffu);
        expected[i] = actual[i];
    }
}

static int equal(void) {
    for (size_t i = 0; i < ARENA_SIZE; ++i) {
        if (actual[i] != expected[i]) return 0;
    }
    return 1;
}

static int check_length(size_t n) {
    for (size_t sa = 0; sa < 16u; ++sa) {
        for (size_t da = 0; da < 16u; ++da) {
            const size_t src = GUARD_SIZE + sa;
            const size_t dst = GUARD_SIZE + 4128u + da;
            reset();
            model_move(expected + dst, expected + src, n);
            if (hal_memcpy_scalar(actual + dst, actual + src, n) != actual + dst || !equal()) return 1;

            reset();
            for (size_t i = 0; i < n; ++i) expected[dst + i] = 0xa5u;
            if (hal_memset_scalar(actual + dst, 0xa5, n) != actual + dst || !equal()) return 2;
        }
    }
    return 0;
}

static int check_overlap(size_t n) {
    const size_t base = GUARD_SIZE + 4097u;
    const size_t shifts[] = {0u, 1u, n == 0u ? 0u : n - 1u, n, n + 1u};

    for (size_t i = 0; i < sizeof(shifts) / sizeof(shifts[0]); ++i) {
        const size_t shift = shifts[i];
        reset();
        model_move(expected + base + shift, expected + base, n);
        if (hal_memmove_scalar(actual + base + shift, actual + base, n) != actual + base + shift || !equal()) return 3;

        reset();
        model_move(expected + base - shift, expected + base, n);
        if (hal_memmove_scalar(actual + base - shift, actual + base, n) != actual + base - shift || !equal()) return 4;
    }
    return 0;
}

int main(void) {
    const hal_memops_backend_t backend = {marked_copy, marked_set, marked_move};
    uint8_t probe[8] = {0};
    if (!hal_memops_begin(current_cpu) || !hal_memops_register(1u, &backend)) return 10;
    fake_cpu = 1u;
    (void)hal_memset(probe, 1, sizeof(probe), BH_MEMCTX_F_DEFAULT);
    if (backend_calls != 0u) return 11; /* never dispatch before freeze */
    if (!hal_memops_freeze() || hal_memops_register(0u, &backend)) return 12;
    (void)hal_memset(probe, 2, sizeof(probe), BH_MEMCTX_F_DEFAULT);
    if (backend_calls != 1u) return 13;
    (void)hal_memset(probe, 3, sizeof(probe), BH_MEMCTX_F_IRQ_SAFE);
    (void)hal_memcpy(probe, probe, 0u, BH_MEMCTX_F_EARLY_BOOT);
    if (backend_calls != 1u) return 14; /* emergency contexts are always scalar */
    fake_cpu = 0u;
    (void)hal_memmove(probe, probe, 0u, BH_MEMCTX_F_DEFAULT);
    if (backend_calls != 1u) return 15; /* unregistered core is scalar */
    static const size_t edges[] = {511u, 512u, 513u, 1023u, 1024u, 1025u, 4095u, 4096u, 4097u};
    for (size_t n = 0; n <= 256u; ++n) {
        int rc = check_length(n);
        if (rc != 0 || (rc = check_overlap(n)) != 0) return rc;
    }
    for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
        int rc = check_length(edges[i]);
        if (rc != 0 || (rc = check_overlap(edges[i])) != 0) return rc;
    }
    puts("memops conformance passed");
    return 0;
}
