#ifndef BHARAT_SYSCALL_STATS_H
#define BHARAT_SYSCALL_STATS_H

#include <stdint.h>

typedef struct bh_syscall_stats {
    uint64_t total_calls;
    uint64_t fast_calls;
    uint64_t slow_calls;
} bh_syscall_stats_t;

void bh_syscall_stats_inc_total(uint32_t core_id);
void bh_syscall_stats_inc_fast(uint32_t core_id);
void bh_syscall_stats_inc_slow(uint32_t core_id);

#endif /* BHARAT_SYSCALL_STATS_H */
