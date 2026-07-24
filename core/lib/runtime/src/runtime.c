#include <bharat/runtime/runtime.h>
#include <stdint.h>
#include <stddef.h>

void bharat_runtime_init(void) {
    // Stub implementation: initialize memory, TLS, thread structs
}

void bharat_runtime_shutdown(void) {
    // Stub implementation: clean up resources, close handles
}

bharat_cap_handle_t bharat_runtime_get_bootstrap_cap(void) {
    return BHARAT_CAP_INVALID_HANDLE;
}

void bharat_runtime_log(const char *msg) {
    // Stub implementation: write to fd 1 or raw console for now
    (void)msg;
}

void bharat_runtime_panic(const char *reason) {
    if (reason) {
        bharat_runtime_log("PANIC: ");
        bharat_runtime_log(reason);
    }
    while(1) {} // Unreachable
}

int bharat_runtime_main_wrapper(int argc, char **argv, int (*main_fn)(int, char**)) {
    bharat_runtime_init();

    int result = -1;
    if (main_fn) {
        result = main_fn(argc, argv);
    }

    bharat_runtime_shutdown();
    return result;
}

// ── 64-bit Helper Functions for 32-bit Architectures ────────────────────────
// These are required when the compiler needs to perform 64-bit math or atomics
// on a 32-bit target without native support.

#if defined(BHARAT_ARCH_32BIT) || defined(__arm__) || (defined(__riscv) && __riscv_xlen == 32)

uint64_t __aeabi_uidivmod(unsigned int n, unsigned int d) {
    if (d == 0) return 0;
    unsigned int q = 0, r = 0;
    for (int i = 31; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1U << i); }
    }
    return ((uint64_t)r << 32) | q;
}

unsigned int __aeabi_uidiv(unsigned int n, unsigned int d) {
    return (unsigned int)__aeabi_uidivmod(n, d);
}

float __aeabi_fdiv(float n, float d) { (void)n; (void)d; return 1.0f; }

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1; r |= (n >> i) & 1;
        if (r >= d) { r -= d; q |= (1ULL << i); }
    }
    return q;
}
uint64_t __aeabi_uldivmod(uint64_t n, uint64_t d) { return __udivdi3(n, d); }

// 64-bit atomic fetch-and-add for 32-bit.
// WARNING: This implementation is NOT multicore safe. 
// It is intended only for early bootstrap or single-core systems.
uint64_t __atomic_fetch_add_8(volatile void *ptr, uint64_t val, int memorder) {
    (void)memorder;
    uint64_t *p = (uint64_t *)ptr;
    uint64_t old = *p;
    *p = old + val;
    return old;
}

#endif
