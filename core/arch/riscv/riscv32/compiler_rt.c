#include <stdint.h>
#include <stddef.h>

extern uint64_t __umoddi3(uint64_t n, uint64_t d);

uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0;
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r <<= 1;
        r |= (n >> i) & 1ULL;
        if (r >= d) {
            r -= d;
            q |= (1ULL << i);
        }
    }
    return q;
}

int64_t __divdi3(int64_t n, int64_t d) {
    int neg = 0;
    if (n < 0) { n = -n; neg = !neg; }
    if (d < 0) { d = -d; neg = !neg; }
    uint64_t q = __udivdi3((uint64_t)n, (uint64_t)d);
    return neg ? -(int64_t)q : (int64_t)q;
}

int64_t __moddi3(int64_t n, int64_t d) {
    int neg = (n < 0);
    if (n < 0) n = -n;
    if (d < 0) d = -d;
    uint64_t r = __umoddi3((uint64_t)n, (uint64_t)d);
    return neg ? -(int64_t)r : (int64_t)r;
}
