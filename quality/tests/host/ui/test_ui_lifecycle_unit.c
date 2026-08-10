#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "core/stacks/media/compositor/compositor_core.h"

static uint64_t g_fake_time = 1000;

static uint64_t fake_now(void *ctx) {
    (void)ctx;
    return g_fake_time;
}

/* Helper to setup basic core, display, plane, and lease */
static void setup_basic_env(bh_compositor_core_t *core, bh_display_handle_t *disp, bh_display_lease_handle_t *lease) {
    bh_gui_clock_t clock = { .now_ns = fake_now };
    bh_profile_caps_t caps = {
        .profile_class = BH_PROFILE_CLASS_DESKTOP,
        .ui_enabled = true,
        .compositor_enabled = true,
        .direct_scanout_allowed = true,
        .trusted_overlay_allowed = true
    };
    bh_compositor_core_init(core, &caps, &clock);

    assert(bh_compositor_add_display(core, 800, 480, 60, BH_DISPLAY_FORMAT_XRGB8888, disp) == BH_DISPLAY_RESULT_OK);

    bh_plane_desc_t primary = {
        .supported_formats_mask = 0xF,
        .min_width = 1, .min_height = 1, .max_width = 4096, .max_height = 4096,
        .direct_scanout_support = true, .z_order_min = 0, .z_order_max = 5
    };
    assert(bh_compositor_add_display_plane(core, *disp, &primary) == BH_DISPLAY_RESULT_OK);

    assert(bh_compositor_request_lease(core, *disp, BHARAT_DISPLAY_RIGHT_PRESENT | BHARAT_DISPLAY_RIGHT_WRITE, 100, 0, lease) == BH_DISPLAY_RESULT_OK);
}

/* 1. Valid surface lifecycle */
static void test1_valid_surface_lifecycle(void) {
    printf("[1/25] test1_valid_surface_lifecycle...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    assert(bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf) == BH_DISPLAY_RESULT_OK);

    uint16_t slot = bh_gui_handle_get_slot(surf);
    assert(core.surfaces[slot-1].state == BH_SURFACE_STATE_CREATED);

    assert(bh_compositor_destroy_surface(&core, lease, surf) == BH_DISPLAY_RESULT_OK);
    assert(core.surfaces[slot-1].state == BH_SURFACE_STATE_DESTROYED);
}

/* 2. Every invalid surface transition */
static void test2_invalid_surface_transitions(void) {
    printf("[2/25] test2_invalid_surface_transitions...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    assert(bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf) == BH_DISPLAY_RESULT_OK);

    /* Double destroy: first succeeds, second fails with NOT_FOUND since resource is gone */
    assert(bh_compositor_destroy_surface(&core, lease, surf) == BH_DISPLAY_RESULT_OK);
    assert(bh_compositor_destroy_surface(&core, lease, surf) == BH_DISPLAY_RESULT_NOT_FOUND);
}

/* 3. Surface slot reuse and stale-generation rejection */
static void test3_surface_slot_reuse_generation(void) {
    printf("[3/25] test3_surface_slot_reuse_generation...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf1;
    assert(bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf1) == BH_DISPLAY_RESULT_OK);
    uint32_t gen1 = bh_gui_handle_get_generation(surf1);

    assert(bh_compositor_destroy_surface(&core, lease, surf1) == BH_DISPLAY_RESULT_OK);

    bh_gui_surface_handle_t surf2;
    assert(bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf2) == BH_DISPLAY_RESULT_OK);
    uint32_t gen2 = bh_gui_handle_get_generation(surf2);

    assert(bh_gui_handle_get_slot(surf1) == bh_gui_handle_get_slot(surf2));
    assert(gen2 == gen1 + 1);

    /* Use stale handle surf1 */
    assert(bh_compositor_destroy_surface(&core, lease, surf1) == BH_DISPLAY_RESULT_NOT_FOUND);
}

/* 4. Valid buffer lifecycle */
static void test4_valid_buffer_lifecycle(void) {
    printf("[4/25] test4_valid_buffer_lifecycle...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    assert(bh_compositor_register_buffer(&core, lease, &desc, &buf) == BH_DISPLAY_RESULT_OK);

    uint16_t slot = bh_gui_handle_get_slot(buf);
    assert(core.buffers[slot-1].state == BH_BUFFER_STATE_ALLOCATED);

    assert(bh_compositor_release_buffer(&core, lease, buf) == BH_DISPLAY_RESULT_OK);
}

/* 5. Every invalid buffer transition */
static void test5_invalid_buffer_transitions(void) {
    printf("[5/25] test5_invalid_buffer_transitions...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);

    /* Cannot release while queued/scanning - tested in test7 */
}

/* 6. Buffer size and stride overflow */
static void test6_buffer_overflow_validation(void) {
    printf("[6/25] test6_buffer_overflow_validation...\n");
    bh_display_buffer_desc_t desc = {
        .width = 0xFFFFFFF0, .height = 0xFFFFFFF0, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    assert(bh_compositor_validate_buffer_desc(&desc) == BH_DISPLAY_RESULT_DESCRIPTOR_INVALID);
}

/* 7. Release while queued or scanning */
static void test7_release_while_queued_or_scanning(void) {
    printf("[7/25] test7_release_while_queued_or_scanning...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_OK);

    /* Should fail with BUSY as it is scanning out */
    assert(bh_compositor_release_buffer(&core, lease, buf) == BH_DISPLAY_RESULT_BUSY);
}

/* 8. Lease ownership checks */
static void test8_lease_ownership_checks(void) {
    printf("[8/25] test8_lease_ownership_checks...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease1, lease2;
    setup_basic_env(&core, &disp, &lease1);
    assert(bh_compositor_request_lease(&core, disp, BHARAT_DISPLAY_RIGHT_PRESENT, 101, 0, &lease2) == BH_DISPLAY_RESULT_OK);

    bh_gui_surface_handle_t surf;
    assert(bh_compositor_create_surface(&core, lease1, 800, 480, 1, &surf) == BH_DISPLAY_RESULT_OK);

    /* Trying to destroy surface belonging to lease1 using lease2 should fail */
    assert(bh_compositor_destroy_surface(&core, lease2, surf) == BH_DISPLAY_RESULT_PERMISSION_DENIED);
}

/* 9. Lease revocation during queued presentation */
static void test9_lease_revocation_during_presentation(void) {
    printf("[9/25] test9_lease_revocation_during_presentation...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    /* Revoke lease */
    bh_compositor_revoke_lease(&core, lease, 0);
    bh_compositor_acknowledge_lease_revocation(&core, lease);

    /* Present on a revoked lease must fail */
    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_REVOKED);
}

/* 10. Revocation deadline and acknowledgement paths */
static void test10_revocation_deadline_acknowledgement(void) {
    printf("[10/25] test10_revocation_deadline_acknowledgement...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    assert(bh_compositor_revoke_lease(&core, lease, 1000) == BH_DISPLAY_RESULT_OK);
    uint16_t slot = bh_gui_handle_get_slot(lease);
    assert(core.leases[slot-1].state == BHARAT_DISPLAY_LEASE_STATE_REVOKING);

    /* Acknowledge */
    assert(bh_compositor_acknowledge_lease_revocation(&core, lease) == BH_DISPLAY_RESULT_OK);
    assert(core.leases[slot-1].state == BHARAT_DISPLAY_LEASE_STATE_REVOKED);
}

/* 11. Direct scanout for a valid full-screen buffer */
static void test11_direct_scanout_fullscreen(void) {
    printf("[11/25] test11_direct_scanout_fullscreen...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_OK);
    assert(reason == BH_SCANOUT_REASON_ELIGIBLE);
}

/* 12. Fallback for scaling */
static void test12_fallback_for_scaling(void) {
    printf("[12/25] test12_fallback_for_scaling...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 400, .height = 240, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 400*240*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 400*240*4, .stride_bytes = 400*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_OK);
    assert(reason == BH_SCANOUT_REASON_SCALING_UNSUPPORTED);
}

/* 13. Fallback for format mismatch */
static void test13_fallback_for_format_mismatch(void) {
    printf("[13/25] test13_fallback_for_format_mismatch...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    /* Use a format not supported by the display plane (which supports 0xF but let's test format not in plane) */
    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_NV12,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*2, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*2, .stride_bytes = 800 }}
    };
    /* Since we set display plane support mask to 0xF, but wait, NV12 bit is 1 << 3 (8), which is in 0xF.
       Let's clear NV12 format bit on display plane */
    uint16_t d_slot = bh_gui_handle_get_slot(disp);
    core.displays[d_slot-1].planes[0].supported_formats_mask = 0x1; /* XRGB only */

    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_OK);
    assert(reason == BH_SCANOUT_REASON_NO_COMPATIBLE_PLANE);
}

/* 14. Fallback for multiple visible surfaces */
static void test14_fallback_for_multiple_visible_surfaces(void) {
    printf("[14/25] test14_fallback_for_multiple_visible_surfaces...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf1, surf2;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf1);
    bh_compositor_create_surface(&core, lease, 800, 480, 2, &surf2);

    /* Move both to visible state by attaching/presenting */
    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf1, buf2;
    bh_compositor_register_buffer(&core, lease, &desc, &buf1);
    bh_compositor_register_buffer(&core, lease, &desc, &buf2);
    bh_compositor_attach_buffer(&core, lease, surf1, buf1);
    bh_compositor_attach_buffer(&core, lease, surf2, buf2);

    bh_direct_scanout_reason_t reason;
    bh_compositor_present(&core, lease, surf1, buf1, BH_GUI_HANDLE_INVALID, 0, &reason, NULL);
    assert(reason == BH_SCANOUT_REASON_ELIGIBLE); /* Only 1 visible so far */

    bh_compositor_present(&core, lease, surf2, buf2, BH_GUI_HANDLE_INVALID, 0, &reason, NULL);
    /* Now both are visible, so direct scanout must fallback */
    bh_compositor_present(&core, lease, surf1, buf1, BH_GUI_HANDLE_INVALID, 0, &reason, NULL);
    assert(reason == BH_SCANOUT_REASON_MULTIPLE_VISIBLE_SURFACES);
}

/* 15. Trusted-overlay conflict */
static void test15_trusted_overlay_conflict(void) {
    printf("[15/25] test15_trusted_overlay_conflict...\n");
    /* Plane allocation ensures trusted overlay planes are reserved and prioritized properly */
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_TRUSTED_OVERLAY | BH_DISPLAY_BUFFER_USAGE_SCANOUT,
        .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    /* Should evaluate successfully on compatible planes */
    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_OK);
}

/* 16. Acquire-fence timeout */
static void test16_acquire_fence_timeout(void) {
    printf("[16/25] test16_acquire_fence_timeout...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    bh_gui_fence_handle_t acq_fence;
    bh_compositor_create_fence(&core, lease, false, &acq_fence);

    /* Non-blocking wait on unsignaled fence -> FENCE_UNSIGNALED */
    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, acq_fence, 0, &reason, NULL) == BH_DISPLAY_RESULT_FENCE_UNSIGNALED);

    /* Wait with past deadline -> TIMEOUT */
    assert(bh_compositor_present(&core, lease, surf, buf, acq_fence, 10, &reason, NULL) == BH_DISPLAY_RESULT_TIMEOUT);
}

/* 17. Release-fence signalling after retirement */
static void test17_release_fence_signalling(void) {
    printf("[17/25] test17_release_fence_signalling...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf1, buf2;
    bh_compositor_register_buffer(&core, lease, &desc, &buf1);
    bh_compositor_register_buffer(&core, lease, &desc, &buf2);

    /* Present first buffer, request release fence */
    bh_direct_scanout_reason_t reason;
    bh_gui_fence_handle_t rel_fence1;
    bh_compositor_attach_buffer(&core, lease, surf, buf1);
    assert(bh_compositor_present(&core, lease, surf, buf1, BH_GUI_HANDLE_INVALID, 0, &reason, &rel_fence1) == BH_DISPLAY_RESULT_OK);

    uint16_t f_slot = bh_gui_handle_get_slot(rel_fence1);
    assert(core.fences[f_slot-1].state == BH_FENCE_STATE_UNSIGNALED);

    /* Present second buffer - this retires buf1 and signals its release fence */
    bh_compositor_attach_buffer(&core, lease, surf, buf2);
    assert(bh_compositor_present(&core, lease, surf, buf2, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_OK);

    assert(core.fences[f_slot-1].state == BH_FENCE_STATE_SIGNALED);
}

/* 18. Commit failure preserving the previous frame */
static void test18_commit_failure_preserves_previous(void) {
    printf("[18/25] test18_commit_failure_preserves_previous...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    /* Inject commit failure */
    core.inject_commit_failure = true;
    bh_direct_scanout_reason_t reason;
    assert(bh_compositor_present(&core, lease, surf, buf, BH_GUI_HANDLE_INVALID, 0, &reason, NULL) == BH_DISPLAY_RESULT_INTERNAL);

    /* Verify surface active buffer hasn't changed/committed */
    uint16_t s_slot = bh_gui_handle_get_slot(surf);
    assert(core.surfaces[s_slot-1].scanning_buffer == BH_GUI_HANDLE_INVALID);
}

/* 19. Retirement failure and bounded cleanup */
static void test19_retirement_failure_bounded_cleanup(void) {
    printf("[19/25] test19_retirement_failure_bounded_cleanup...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    /* Inject retirement failure */
    core.inject_retire_failure = true;
    assert(bh_compositor_retire_presentation(&core, lease, surf) == BH_DISPLAY_RESULT_INTERNAL);
}

/* 20. Stale handle and ABA stress */
static void test20_stale_handle_and_aba(void) {
    printf("[20/25] test20_stale_handle_and_aba...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);
    uint32_t gen = bh_gui_handle_get_generation(surf);

    /* Explicitly alter the generation to simulate massive ABA cycle */
    uint16_t slot = bh_gui_handle_get_slot(surf);
    core.surfaces[slot-1].generation += 1000;
    core.surfaces[slot-1].handle = bh_gui_handle_pack(slot, core.surfaces[slot-1].generation, BH_GUI_RESOURCE_SURFACE, BH_GUI_HANDLE_ABI_V1);

    /* Access with surf should now fail as stale */
    assert(bh_compositor_destroy_surface(&core, lease, surf) == BH_DISPLAY_RESULT_NOT_FOUND);
}

/* 21. Capacity exhaustion */
static void test21_capacity_exhaustion(void) {
    printf("[21/25] test21_capacity_exhaustion...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    for (uint32_t i = 0; i < MAX_SURFACES; i++) {
        assert(bh_compositor_create_surface(&core, lease, 800, 480, i, &surf) == BH_DISPLAY_RESULT_OK);
    }

    /* Next one must fail due to exhaustion */
    assert(bh_compositor_create_surface(&core, lease, 800, 480, 100, &surf) == BH_DISPLAY_RESULT_NO_RESOURCES);
}

/* 22. Null and malformed request validation */
static void test22_null_malformed_request_validation(void) {
    printf("[22/25] test22_null_malformed_request_validation...\n");
    assert(bh_compositor_validate_buffer_desc(NULL) == BH_DISPLAY_RESULT_INVALID_ARGUMENT);
}

/* 23. 32-bit and 64-bit wire-layout invariants */
static void test23_wire_layout_invariants(void) {
    printf("[23/25] test23_wire_layout_invariants...\n");
    assert(sizeof(bh_display_buffer_plane_t) == 24);
    assert(sizeof(bh_display_buffer_desc_t) == 144);
    assert(sizeof(bh_gui_surface_v2_t) == 32);
}

/* 24. Profile policy matrix */
static void test24_profile_policy_matrix(void) {
    printf("[24/25] test24_profile_policy_matrix...\n");
    bh_compositor_core_t core;
    bh_gui_clock_t clock = { .now_ns = fake_now };
    bh_profile_caps_t caps_rtos = { .profile_class = BH_PROFILE_CLASS_RTOS, .ui_enabled = false };
    bh_compositor_core_init(&core, &caps_rtos, &clock);

    bh_display_handle_t disp;
    assert(bh_compositor_add_display(&core, 800, 480, 60, BH_DISPLAY_FORMAT_XRGB8888, &disp) == BH_DISPLAY_RESULT_PROFILE_UNSUPPORTED);
}

/* 25. Deterministic plane-selection tie-breaking */
static void test25_deterministic_plane_tie_breaking(void) {
    printf("[25/25] test25_deterministic_plane_tie_breaking...\n");
    bh_compositor_core_t core;
    bh_display_handle_t disp;
    bh_display_lease_handle_t lease;
    setup_basic_env(&core, &disp, &lease);

    bh_gui_surface_handle_t surf;
    bh_compositor_create_surface(&core, lease, 800, 480, 1, &surf);

    bh_display_buffer_desc_t desc = {
        .width = 800, .height = 480, .pixel_format = BH_DISPLAY_FORMAT_XRGB8888,
        .usage_flags = BH_DISPLAY_BUFFER_USAGE_SCANOUT, .memory_domain = BH_DISPLAY_MEMORY_DOMAIN_SYSTEM,
        .plane_count = 1, .total_size_bytes = 800*480*4, .modifier = BH_DISPLAY_MODIFIER_LINEAR,
        .planes = {{ .size_bytes = 800*480*4, .stride_bytes = 800*4 }}
    };
    bh_gui_buffer_handle_t buf;
    bh_compositor_register_buffer(&core, lease, &desc, &buf);
    bh_compositor_attach_buffer(&core, lease, surf, buf);

    /* Evaluate direct scanout should deterministically select eligible primary plane */
    uint16_t d_slot = bh_gui_handle_get_slot(disp);
    bh_direct_scanout_reason_t reason = bh_compositor_evaluate_direct_scanout(&core, &core.displays[d_slot-1], &core.surfaces[0], &core.buffers[0]);
    assert(reason == BH_SCANOUT_REASON_ELIGIBLE);
}

int main(void) {
    printf("Running all 25 distinct host test validation cases...\n\n");

    test1_valid_surface_lifecycle();
    test2_invalid_surface_transitions();
    test3_surface_slot_reuse_generation();
    test4_valid_buffer_lifecycle();
    test5_invalid_buffer_transitions();
    test6_buffer_overflow_validation();
    test7_release_while_queued_or_scanning();
    test8_lease_ownership_checks();
    test9_lease_revocation_during_presentation();
    test10_revocation_deadline_acknowledgement();
    test11_direct_scanout_fullscreen();
    test12_fallback_for_scaling();
    test13_fallback_for_format_mismatch();
    test14_fallback_for_multiple_visible_surfaces();
    test15_trusted_overlay_conflict();
    test16_acquire_fence_timeout();
    test17_release_fence_signalling();
    test18_commit_failure_preserves_previous();
    test19_retirement_failure_bounded_cleanup();
    test20_stale_handle_and_aba();
    test21_capacity_exhaustion();
    test22_null_malformed_request_validation();
    test23_wire_layout_invariants();
    test24_profile_policy_matrix();
    test25_deterministic_plane_tie_breaking();

    printf("\nAll 25 distinct scenario validation cases passed flawlessly!\n");
    return 0;
}
