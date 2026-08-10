#ifndef BHARAT_UAPI_GUI_SURFACE_V2_H
#define BHARAT_UAPI_GUI_SURFACE_V2_H

#include <stdint.h>
#include "bharat/uapi/display/display_v2.h"

/* Surface v2 State Machine Definitions */
typedef enum {
    BH_SURFACE_STATE_FREE = 0,
    BH_SURFACE_STATE_CREATED = 1,
    BH_SURFACE_STATE_CONFIGURED = 2,
    BH_SURFACE_STATE_VISIBLE = 3,
    BH_SURFACE_STATE_OCCLUDED = 4,
    BH_SURFACE_STATE_DESTROY_PENDING = 5,
    BH_SURFACE_STATE_DESTROYED = 6,
} bh_surface_state_v2_t;

/* Surface descriptor v2 */
typedef struct {
    bh_gui_surface_handle_t handle;
    uint32_t owner_pid;
    uint32_t z_order;
    uint32_t width;
    uint32_t height;
    uint32_t state; /* bh_surface_state_v2_t */
    uint32_t reserved;
} bh_gui_surface_v2_t;

#endif /* BHARAT_UAPI_GUI_SURFACE_V2_H */
