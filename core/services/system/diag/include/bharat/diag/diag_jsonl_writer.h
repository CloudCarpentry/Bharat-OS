/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_DIAG_JSONL_WRITER_H
#define BHARAT_DIAG_JSONL_WRITER_H
#include "bharat/diag/diag_ring.h"
typedef bh_status_t (*bh_diag_text_write_t)(void *context, const char *text, uint32_t size);
typedef struct bh_diag_jsonl_sink { bh_diag_text_write_t write; void *context; } bh_diag_jsonl_sink_t;
bh_status_t bh_diag_jsonl_write(void *context, const bh_diag_record_t *record);
#endif
