#ifndef BHARATLIBC_BSYS_TYPES_H
#define BHARATLIBC_BSYS_TYPES_H

#include <standard/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t tv_sec;
    uint64_t tv_nsec;
} bh_bsys_timespec_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_BSYS_TYPES_H */
