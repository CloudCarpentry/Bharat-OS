#include <stdint.h>
#include <stddef.h>
#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/syscall/bh_syscall.h>

#include <bharat/uapi/init/bootstrap.h>
#include <bharat/runtime/runtime.h>
#include <bharat/uapi/services/service_ids.h>

extern int main(int argc, char* argv[]);

/* TODO: Implement real TLS initialization for __thread variables */
static void init_tls(void) {
    /* Dummy for now */
}

__attribute__((used))
void _start(const bharat_user_startup_t *startup) {
    init_tls();

    bharat_runtime_init(startup);
    if (startup && startup->bootstrap.flags == BHARAT_SERVICE_NAMESVC) {
        bharat_runtime_log("NAMESVC_USER_ENTRY\n");
    }

    int ret = main(
        startup ? (int)startup->argc : 0,
        startup ? (char **)startup->argv : NULL
    );

    bharat_syscall(SYSCALL_THREAD_EXIT, ret, 0, 0, 0, 0, 0);

    while (1) {}
}

#if defined(__arm__) && !defined(__aarch64__)
static uint64_t bharat_udiv64(uint64_t num, uint64_t den) {
    uint64_t q = 0;
    uint64_t r = 0;
    int i;
    if (den == 0u) {
        return 0u;
    }
    for (i = 63; i >= 0; --i) {
        r = (r << 1) | ((num >> i) & 1u);
        if (r >= den) {
            r -= den;
            q |= (1ull << i);
        }
    }
    return q;
}

__attribute__((weak)) void __aeabi_memcpy(void* dest, const void* src, size_t n) {
    size_t i;
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (i = 0; i < n; ++i) {
        d[i] = s[i];
    }
}

__attribute__((weak)) void __aeabi_memcpy4(void* dest, const void* src, size_t n) { __aeabi_memcpy(dest, src, n); }
__attribute__((weak)) void __aeabi_memcpy8(void* dest, const void* src, size_t n) { __aeabi_memcpy(dest, src, n); }

__attribute__((weak)) void __aeabi_memclr(void* dest, size_t n) {
    size_t i;
    unsigned char* d = (unsigned char*)dest;
    for (i = 0; i < n; ++i) {
        d[i] = 0;
    }
}

__attribute__((weak)) void __aeabi_memclr4(void* dest, size_t n) { __aeabi_memclr(dest, n); }
__attribute__((weak)) void __aeabi_memclr8(void* dest, size_t n) { __aeabi_memclr(dest, n); }

__attribute__((weak)) uint64_t __aeabi_uldivmod(uint64_t numerator, uint64_t denominator) {
    return bharat_udiv64(numerator, denominator);
}
#endif

#if (__SIZEOF_POINTER__ == 4)
static uint64_t bharat_udiv64_generic(uint64_t num, uint64_t den) {
    uint64_t q = 0;
    uint64_t r = 0;
    int i;
    if (den == 0u) {
        return 0u;
    }
    for (i = 63; i >= 0; --i) {
        r = (r << 1) | ((num >> i) & 1u);
        if (r >= den) {
            r -= den;
            q |= (1ull << i);
        }
    }
    return q;
}

__attribute__((weak)) uint64_t __udivdi3(uint64_t numerator, uint64_t denominator) {
    return bharat_udiv64_generic(numerator, denominator);
}
#endif
__attribute__((weak)) int64_t __divdi3(int64_t n, int64_t d) {
    int neg = 0;
    if (n < 0) {
        n = -n;
        neg = !neg;
    }
    if (d < 0) {
        d = -d;
        neg = !neg;
    }
    uint64_t q = __udivdi3((uint64_t)n, (uint64_t)d);
    if (neg) {
        return -(int64_t)q;
    }
    return (int64_t)q;
}
