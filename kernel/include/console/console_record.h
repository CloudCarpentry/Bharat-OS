#pragma once

#include "console_base_types.h"
#include "console_types.h"

#define CONSOLE_MAX_RECORD_TEXT 224

#define CON_RECORD_FLAG_TRUNCATED   ((console_flags_t)1u << 0)
#define CON_RECORD_FLAG_REPLAYED    ((console_flags_t)1u << 1)
#define CON_RECORD_FLAG_PANIC_PATH  ((console_flags_t)1u << 2)
#define CON_RECORD_FLAG_RAW_WRITE   ((console_flags_t)1u << 3)

typedef struct {
    console_seq_t seq_no;
    console_time_t timestamp_ns;
    console_flags_t flags;
    console_len_t text_len;
    console_cpu_id_t cpu_id;
    console_level_t level;
    char text[CONSOLE_MAX_RECORD_TEXT];
} console_record_t;
