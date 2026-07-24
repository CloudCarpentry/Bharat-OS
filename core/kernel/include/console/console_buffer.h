#pragma once

#include "console_base_types.h"
#include "console_record.h"
#include "console_backend.h"

#define CONSOLE_RING_CAPACITY 1024

typedef struct {
    console_record_t records[CONSOLE_RING_CAPACITY];
    console_index_t head;
    console_index_t count;
    console_seq_t next_seq;
    uint32_t dropped_records;
} console_ring_t;

void console_buffer_init(void);
bool console_buffer_push(console_record_t *rec);
bool console_buffer_peek_oldest(console_record_t *out, console_index_t logical_index);
size_t console_buffer_replay_to_backend(struct console_backend *backend, bool include_below_min);
uint32_t console_buffer_dropped_count(void);
