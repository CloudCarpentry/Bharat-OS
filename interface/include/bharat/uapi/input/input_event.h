#ifndef BHARAT_UAPI_INPUT_EVENT_H
#define BHARAT_UAPI_INPUT_EVENT_H

#include <stdint.h>

/**
 * Input event types.
 */
typedef enum {
    BHARAT_INPUT_KEY = 1,
    BHARAT_INPUT_POINTER = 2,
    BHARAT_INPUT_TOUCH = 3,
    BHARAT_INPUT_SCROLL = 4,
    BHARAT_INPUT_TEXT = 5,
    BHARAT_INPUT_DEVICE = 6,
} bharat_input_event_type_t;

/**
 * Generic input event structure.
 */
typedef struct {
    uint64_t timestamp_ns;
    uint16_t type;  /* bharat_input_event_type_t */
    uint16_t code;  /* Key code or axis */
    int32_t value;  /* Pressed state or coordinate */
    uint32_t device_id;
} bharat_input_event_t;

typedef struct {
    uint64_t timestamp_ticks;
    uint32_t device_id;
    uint16_t type;
    uint16_t code;
    int32_t value;
    uint32_t sequence;
} bh_input_event_t;

_Static_assert(sizeof(bh_input_event_t) == 24, "input ABI size");

#endif /* BHARAT_UAPI_INPUT_EVENT_H */
