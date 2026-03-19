#include "arch/fastops_dispatch.h"
#include "arch/arch_cpu_caps.h"
#include <stddef.h>

// Forward declarations from kernel string functions (kernel/src/lib/string.c)
void *memset(void *dest, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

// Generic Fallbacks
static void generic_page_zero(void *page) {
    memset(page, 0, 4096);
}

static void *generic_memcpy(void *dst, const void *src, size_t n) {
    return memcpy(dst, src, n);
}

static void *generic_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}

// Function Pointers default to generic fallbacks
fast_page_zero_fn_t fast_page_zero = generic_page_zero;
fast_memcpy_fn_t fast_memcpy = generic_memcpy;
fast_memset_fn_t fast_memset = generic_memset;

// Optimized Stubs (to be replaced with real inline asm or optimized routines)
static void optimized_page_zero_vector(void *page) {
    // Vector optimized page zero stub
    memset(page, 0, 4096);
}

void fastops_dispatch_init(void) {
    const arch_cpu_caps_record_t *sys_caps = arch_cpu_caps_system_all();

    // Check if VECTOR features are present on ALL CPUs
    if (arch_cpu_caps_test(&sys_caps->usable, ARCH_CPU_FEAT_COMMON_VECTOR)) {
        fast_page_zero = optimized_page_zero_vector;
    }
}
