/* SPDX-License-Identifier: MIT */
#include "diag/event.h"
#include "diag/counters.h"
#include "diag/health.h"
#include "diag/boot_evidence.h"
#include <assert.h>
int main(void){assert(sizeof(bh_diag_event_header_t)==40);assert(sizeof(bh_diag_counter_t)==16);assert(sizeof(bh_diag_health_snapshot_t)==48);assert(sizeof(bh_boot_stage_evidence_t)==24);return 0;}
