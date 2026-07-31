#ifndef BHARAT_COMPOSITOR_CORE_H
#define BHARAT_COMPOSITOR_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/gui/surface_v2.h"

#define MAX_DISPLAYS 4u
#define MAX_LEASES   8u
#define MAX_SURFACES 16u
#define MAX_BUFFERS  32u
#define MAX_FENCES   64u

/* Clock structure for absolute monotonic deadlines */
typedef struct {
    uint64_t (*now_ns)(void *ctx);
    void *ctx;
} bh_gui_clock_t;

/* Profile Capability Descriptor */
typedef enum {
    BH_PROFILE_CLASS_TINY = 1,
    BH_PROFILE_CLASS_RTOS = 2,
    BH_PROFILE_CLASS_EDGE = 3,
    BH_PROFILE_CLASS_DESKTOP = 4,
    BH_PROFILE_CLASS_AUTOMOTIVE = 5,
} bh_profile_class_t;

typedef struct {
    bh_profile_class_t profile_class;
    bool ui_enabled;
    bool compositor_enabled;
    bool direct_scanout_allowed;
    bool trusted_overlay_allowed;
} bh_profile_caps_t;

/* Plane Descriptor */
typedef struct {
    uint32_t plane_id;
    uint32_t supported_formats_mask;
    uint32_t min_width;
    uint32_t min_height;
    uint32_t max_width;
    uint32_t max_height;
    bool scaling_support;
    bool rotation_support;
    bool secure_overlay_support;
    bool cursor_plane_support;
    bool direct_scanout_support;
    int32_t z_order_min;
    int32_t z_order_max;
} bh_plane_desc_t;

/* Core Object Entries in Static Bounded Tables */
typedef struct {
    bool active;
    uint32_t generation;
    bh_display_handle_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t refresh_hz;
    uint32_t pixel_format;
    uint32_t flags;
    bh_display_lease_handle_t active_lease;
    uint32_t plane_count;
    bh_plane_desc_t planes[BH_DISPLAY_MAX_PLANES];
} bh_display_entry_t;

typedef struct {
    bool active;
    uint32_t generation;
    bh_display_lease_handle_t handle;
    bh_display_handle_t display_handle;
    uint32_t rights;
    uint32_t state; /* bh_display_lease_state_t */
    uint32_t client_pid;
    uint64_t client_endpoint;
    bh_monotonic_deadline_ns_t revocation_deadline;
} bh_lease_entry_t;

typedef struct {
    bool active;
    uint32_t generation;
    bh_gui_surface_handle_t handle;
    bh_display_lease_handle_t lease_handle;
    uint32_t width;
    uint32_t height;
    uint32_t z_order;
    uint32_t state; /* bh_surface_state_v2_t */
    bh_gui_buffer_handle_t attached_buffer;
    bh_gui_buffer_handle_t scanning_buffer;
    bh_gui_buffer_handle_t retired_buffer;
    uint64_t frame_counter;
} bh_surface_entry_t;

typedef struct {
    bool active;
    uint32_t generation;
    bh_gui_buffer_handle_t handle;
    bh_display_lease_handle_t lease_handle;
    bh_display_buffer_desc_t desc;
    uint32_t state; /* bh_buffer_state_v2_t */
    bh_gui_surface_handle_t attached_surface;
    bh_gui_fence_handle_t release_fence;
} bh_buffer_entry_t;

typedef struct {
    bool active;
    uint32_t generation;
    bh_gui_fence_handle_t handle;
    bh_display_lease_handle_t lease_handle;
    uint32_t state; /* bh_fence_state_v2_t */
    bool is_release_fence;
} bh_fence_entry_t;

/* Transport-Neutral Compositor Core Instance */
typedef struct {
    bh_display_entry_t displays[MAX_DISPLAYS];
    bh_lease_entry_t leases[MAX_LEASES];
    bh_surface_entry_t surfaces[MAX_SURFACES];
    bh_buffer_entry_t buffers[MAX_BUFFERS];
    bh_fence_entry_t fences[MAX_FENCES];

    bh_profile_caps_t profile_caps;
    bh_gui_clock_t clock;

    /* Diagnostics/Metrics */
    uint64_t frame_count;
    uint64_t direct_scanout_count;
    uint64_t composition_count;
    uint64_t retirement_count;

    /* Failure Injection Flags */
    bool inject_queue_failure;
    bool inject_commit_failure;
    bool inject_retire_failure;
} bh_compositor_core_t;

/* Public Core API functions */
void bh_compositor_core_init(bh_compositor_core_t *core, const bh_profile_caps_t *caps, const bh_gui_clock_t *clock);

/* Object Lifecycle Functions */
bh_display_result_t bh_compositor_add_display(bh_compositor_core_t *core, uint32_t w, uint32_t h, uint32_t refresh, uint32_t format, bh_display_handle_t *out_handle);
bh_display_result_t bh_compositor_add_display_plane(bh_compositor_core_t *core, bh_display_handle_t disp_handle, const bh_plane_desc_t *plane);

bh_display_result_t bh_compositor_request_lease(bh_compositor_core_t *core, bh_display_handle_t disp, uint32_t requested_rights, uint32_t client_pid, uint64_t endpoint, bh_display_lease_handle_t *out_lease);
bh_display_result_t bh_compositor_revoke_lease(bh_compositor_core_t *core, bh_display_lease_handle_t lease, uint64_t grace_period_ns);
bh_display_result_t bh_compositor_acknowledge_lease_revocation(bh_compositor_core_t *core, bh_display_lease_handle_t lease);
bh_display_result_t bh_compositor_release_lease(bh_compositor_core_t *core, bh_display_lease_handle_t lease);

bh_display_result_t bh_compositor_create_surface(bh_compositor_core_t *core, bh_display_lease_handle_t lease, uint32_t w, uint32_t h, uint32_t z_order, bh_gui_surface_handle_t *out_surface);
bh_display_result_t bh_compositor_destroy_surface(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface);

bh_display_result_t bh_compositor_register_buffer(bh_compositor_core_t *core, bh_display_lease_handle_t lease, const bh_display_buffer_desc_t *desc, bh_gui_buffer_handle_t *out_buffer);
bh_display_result_t bh_compositor_release_buffer(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bh_gui_buffer_handle_t buffer);

bh_display_result_t bh_compositor_attach_buffer(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, bh_gui_buffer_handle_t buffer);

bh_display_result_t bh_compositor_create_fence(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bool is_release, bh_gui_fence_handle_t *out_fence);
bh_display_result_t bh_compositor_signal_fence(bh_compositor_core_t *core, bh_gui_fence_handle_t fence);
bh_display_result_t bh_compositor_wait_fence(bh_compositor_core_t *core, bh_gui_fence_handle_t fence, bh_monotonic_deadline_ns_t deadline);

/* Direct-Scanout and Presentation Transactions */
bh_display_result_t bh_compositor_present(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, bh_gui_buffer_handle_t buffer, bh_gui_fence_handle_t acquire_fence, bh_monotonic_deadline_ns_t deadline, bh_direct_scanout_reason_t *out_reason, bh_gui_fence_handle_t *out_release_fence);
bh_display_result_t bh_compositor_retire_presentation(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface);
bh_display_result_t bh_compositor_query_presentation_status(bh_compositor_core_t *core, bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, uint32_t *out_state, bh_gui_buffer_handle_t *out_active_buffer, uint64_t *out_frame_counter);

/* Helper validators */
bh_display_result_t bh_compositor_validate_buffer_desc(const bh_display_buffer_desc_t *desc);
bh_direct_scanout_reason_t bh_compositor_evaluate_direct_scanout(bh_compositor_core_t *core, bh_display_entry_t *disp, bh_surface_entry_t *surf, bh_buffer_entry_t *buf);

#endif /* BHARAT_COMPOSITOR_CORE_H */
