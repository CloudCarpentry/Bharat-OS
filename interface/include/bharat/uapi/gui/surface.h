#ifndef BHARAT_UAPI_GUI_SURFACE_H
#define BHARAT_UAPI_GUI_SURFACE_H

#include <stdint.h>

/**
 * Surface lifecycle states.
 */
typedef enum {
    BHARAT_SURFACE_STATE_CREATED = 0,
    BHARAT_SURFACE_STATE_VISIBLE = 1,
    BHARAT_SURFACE_STATE_OCCLUDED = 2,
    BHARAT_SURFACE_STATE_FOCUSED = 3,
    BHARAT_SURFACE_STATE_MINIMIZED = 4,
    BHARAT_SURFACE_STATE_DESTROYED = 5,
} bharat_surface_state_t;

/**
 * Surface descriptor.
 */
typedef struct {
    uint64_t surface_id;
    uint32_t owner_pid;
    uint32_t z_order;
    uint32_t width;
    uint32_t height;
    uint32_t state; /* bharat_surface_state_t */
} bharat_gui_surface_t;

#endif /* BHARAT_UAPI_GUI_SURFACE_H */
