/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UAPI_DIAG_COUNTERS_H
#define BHARAT_UAPI_DIAG_COUNTERS_H
#include <stdint.h>
typedef struct bh_diag_counter { uint32_t counter_id; uint32_t flags; uint64_t value; } bh_diag_counter_t;
_Static_assert(sizeof(bh_diag_counter_t) == 16, "counter ABI size");
#endif
