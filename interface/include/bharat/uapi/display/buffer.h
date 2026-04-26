#ifndef BHARAT_UAPI_DISPLAY_BUFFER_H
#define BHARAT_UAPI_DISPLAY_BUFFER_H

#include <stdint.h>

/**
 * Graphics buffer lifecycle states.
 */
typedef enum {
    BHARAT_BUFFER_STATE_ALLOCATED = 0,
    BHARAT_BUFFER_STATE_MAPPED = 1,
    BHARAT_BUFFER_STATE_SUBMITTED = 2,
    BHARAT_BUFFER_STATE_PRESENTED = 3,
    BHARAT_BUFFER_STATE_RELEASED = 4,
} bharat_buffer_lifecycle_t;

/**
 * Standard graphics buffer descriptor for UAPI.
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    uint32_t pixel_format;
    uint64_t buffer_id;
    uint64_t capability_id; /* Tied to capability system */
} bharat_display_buffer_t;

#endif /* BHARAT_UAPI_DISPLAY_BUFFER_H */
