/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_UAPI_DIAG_BOOT_EVIDENCE_H
#define BHARAT_UAPI_DIAG_BOOT_EVIDENCE_H
#include <stdint.h>
typedef enum bh_boot_stage_result { BH_BOOT_STAGE_NOT_OBSERVED = 0, BH_BOOT_STAGE_STARTED, BH_BOOT_STAGE_SUCCEEDED, BH_BOOT_STAGE_FAILED, BH_BOOT_STAGE_SKIPPED } bh_boot_stage_result_t;
typedef struct bh_boot_stage_evidence { uint16_t schema_version; uint16_t stage_id; uint8_t result; uint8_t confidence; uint16_t reserved; uint64_t timestamp_ns; uint64_t event_sequence; } bh_boot_stage_evidence_t;
_Static_assert(sizeof(bh_boot_stage_evidence_t) == 24, "boot evidence ABI size");
#endif
