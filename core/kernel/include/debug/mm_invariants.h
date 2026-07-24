#ifndef BHARAT_MM_INVARIANTS_H
#define BHARAT_MM_INVARIANTS_H

#include <stdint.h>
#include "console/console_core.h"
#include "panic.h"

#define MM_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            console_log(CONSOLE_LEVEL_ERROR, "MM_ASSERT FAILED: %s\n", msg); \
            kernel_panic(msg); \
        } \
    } while (0)

#define MM_WARN(cond, msg) \
    do { \
        if (!(cond)) { \
            console_log(CONSOLE_LEVEL_WARN, "MM_WARN: %s\n", msg); \
        } \
    } while (0)

#ifdef CONFIG_MM_TRACE
#define MM_TRACE(fmt, ...) console_log(CONSOLE_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define MM_TRACE(fmt, ...) do {} while(0)
#endif

struct mm_debug_stats {
    uint64_t aspace_create_calls;
    uint64_t aspace_create_failures;
    uint64_t aspace_rejected_by_profile;
};

extern struct mm_debug_stats mm_stats;

void mm_debug_dump(void);

#endif // BHARAT_MM_INVARIANTS_H
