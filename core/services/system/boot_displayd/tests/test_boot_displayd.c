/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bharat/uapi/display/display_v2.h"
#include "bharat/uapi/boot/boot_events.h"
#include "../display_client.h"

typedef enum {
    BOOT_STATE_UNAVAILABLE,
    BOOT_STATE_INITIALIZING,
    BOOT_STATE_SPLASH_ACTIVE,
    BOOT_STATE_RECOVERY,
    BOOT_STATE_HANDOFF_PENDING,
    BOOT_STATE_RELEASED,
    BOOT_STATE_FAILED
} boot_display_state_t;

typedef struct {
    uint32_t width_px;
    uint32_t height_px;
    uint32_t stride_bytes;
    uint32_t pixel_format;
    void *pixels;
} bharat_tiny_fb_t;

typedef struct {
    boot_display_state_t state;
    bh_showcase_display_session_t session;
    bh_gui_surface_handle_t surface;
    bh_gui_buffer_handle_t buffer;
    void *mapped_pixels;
    bharat_tiny_fb_t fb;
    uint8_t progress_percent;
    bool is_lvgl;
} boot_display_ctx_t;

extern boot_display_state_t boot_displayd_get_state(void);
extern void boot_displayd_set_state_for_test(boot_display_state_t state);
extern int boot_display_init(boot_display_ctx_t *ctx);
extern uint8_t boot_display_stage_to_progress(bh_boot_stage_t stage);
extern void boot_display_handle_record(boot_display_ctx_t *ctx, const bh_boot_event_record_t *rec);
extern void boot_display_update_from_snapshot(boot_display_ctx_t *ctx, const bh_boot_event_snapshot_t *snapshot);
extern void boot_display_handoff(boot_display_ctx_t *ctx);

/* Mock tracking */
static bh_display_result_t mock_open_result = BH_DISPLAY_RESULT_OK;
static uint32_t mock_pixel_format = BH_DISPLAY_FORMAT_XRGB8888;
static bh_display_result_t mock_surface_result = BH_DISPLAY_RESULT_OK;
static bh_display_result_t mock_register_result = BH_DISPLAY_RESULT_OK;
static bh_display_result_t mock_attach_result = BH_DISPLAY_RESULT_OK;

static uint32_t mock_release_buffer_calls = 0;
static uint32_t mock_destroy_surface_calls = 0;
static uint32_t mock_release_lease_calls = 0;

/* Mocks for display_client.h */
bh_display_result_t bh_showcase_display_open(bh_showcase_display_session_t *session) {
    if (mock_open_result == BH_DISPLAY_RESULT_OK) {
        session->display = 1;
        session->lease = 2;
        session->width = 800;
        session->height = 600;
        session->refresh_hz = 60;
        session->pixel_format = mock_pixel_format;
    }
    return mock_open_result;
}

bh_display_result_t bh_client_create_surface(bh_display_lease_handle_t lease, uint32_t width, uint32_t height, uint32_t z_order, bh_gui_surface_handle_t *out_surface) {
    (void)lease; (void)width; (void)height; (void)z_order;
    if (mock_surface_result == BH_DISPLAY_RESULT_OK) {
        *out_surface = 3;
    }
    return mock_surface_result;
}

bh_display_result_t bh_client_destroy_surface(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface) {
    (void)lease; (void)surface;
    mock_destroy_surface_calls++;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_client_register_buffer(bh_display_lease_handle_t lease, bh_display_buffer_desc_t *desc, bh_gui_buffer_handle_t *out_buffer, void **out_mapped) {
    (void)lease; (void)desc;
    if (mock_register_result == BH_DISPLAY_RESULT_OK) {
        *out_buffer = 4;
        *out_mapped = malloc(800 * 600 * 4);
    }
    return mock_register_result;
}

bh_display_result_t bh_client_release_buffer(bh_display_lease_handle_t lease, bh_gui_buffer_handle_t buffer) {
    (void)lease; (void)buffer;
    mock_release_buffer_calls++;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_client_attach_buffer(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, bh_gui_buffer_handle_t buffer) {
    (void)lease; (void)surface; (void)buffer;
    return mock_attach_result;
}

bh_display_result_t bh_client_present_surface(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, bh_gui_buffer_handle_t buffer, bh_gui_fence_handle_t *out_release_fence) {
    (void)lease; (void)surface; (void)buffer;
    *out_release_fence = BH_GUI_HANDLE_INVALID;
    return BH_DISPLAY_RESULT_OK;
}

bh_display_result_t bh_client_release_lease(bh_display_lease_handle_t lease) {
    (void)lease;
    mock_release_lease_calls++;
    return BH_DISPLAY_RESULT_OK;
}

void bharat_runtime_log(const char *msg) {
    (void)msg;
}

void bharat_tiny_ui_init(void *state, bool safe_mode) {
    (void)state; (void)safe_mode;
}

void bharat_tiny_ui_render(const void *fb, const void *state) {
    (void)fb; (void)state;
}

static void reset_mocks(void) {
    mock_open_result = BH_DISPLAY_RESULT_OK;
    mock_pixel_format = BH_DISPLAY_FORMAT_XRGB8888;
    mock_surface_result = BH_DISPLAY_RESULT_OK;
    mock_register_result = BH_DISPLAY_RESULT_OK;
    mock_attach_result = BH_DISPLAY_RESULT_OK;
    mock_release_buffer_calls = 0;
    mock_destroy_surface_calls = 0;
    mock_release_lease_calls = 0;
}

static void test_successful_init(void) {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == 0);
    assert(ctx.state == BOOT_STATE_SPLASH_ACTIVE);
    assert(ctx.fb.width_px == 800);
    assert(ctx.fb.height_px == 600);
    assert(ctx.progress_percent == 0);
    boot_display_handoff(&ctx);
    printf("test_successful_init passed\n");
}

static void test_failed_open(void) {
    reset_mocks();
    mock_open_result = BH_DISPLAY_RESULT_NOT_FOUND;
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == -1);
    assert(ctx.state == BOOT_STATE_UNAVAILABLE);
    printf("test_failed_open passed\n");
}

static void test_unsupported_format(void) {
    reset_mocks();
    mock_pixel_format = BH_DISPLAY_FORMAT_RGB565;
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == -1);
    assert(ctx.state == BOOT_STATE_FAILED);
    printf("test_unsupported_format passed\n");
}

static void test_failed_surface(void) {
    reset_mocks();
    mock_surface_result = BH_DISPLAY_RESULT_NO_RESOURCES;
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == -1);
    assert(ctx.state == BOOT_STATE_FAILED);
    printf("test_failed_surface passed\n");
}

static void test_stage_progress_mapping(void) {
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_EARLY) == 10);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_HAL) == 20);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_SECURITY) == 30);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_MEMORY) == 45);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_SCHEDULER) == 60);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_DRIVERS) == 75);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_SERVICES) == 85);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_USERSPACE) == 95);
    assert(boot_display_stage_to_progress(BH_BOOT_STAGE_READY) == 100);
    printf("test_stage_progress_mapping passed\n");
}

static void test_snapshot_progress_monotonicity(void) {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    boot_display_init(&ctx);

    bh_boot_event_snapshot_t snap = {0};
    snap.total_events = 3;
    snap.count = 3;

    snap.events[0].stage = BH_BOOT_STAGE_EARLY;
    snap.events[0].status = BH_BOOT_STATUS_OK;

    snap.events[1].stage = BH_BOOT_STAGE_MEMORY;
    snap.events[1].status = BH_BOOT_STATUS_OK;

    snap.events[2].stage = BH_BOOT_STAGE_DRIVERS;
    snap.events[2].status = BH_BOOT_STATUS_OK;

    boot_display_update_from_snapshot(&ctx, &snap);
    assert(ctx.progress_percent == 75);
    assert(ctx.state == BOOT_STATE_SPLASH_ACTIVE);

    /* Advance to READY */
    snap.count = 4;
    snap.total_events = 4;
    snap.events[3].stage = BH_BOOT_STAGE_READY;
    snap.events[3].status = BH_BOOT_STATUS_OK;

    boot_display_update_from_snapshot(&ctx, &snap);
    assert(ctx.progress_percent == 100);
    assert(ctx.state == BOOT_STATE_HANDOFF_PENDING);

    boot_display_handoff(&ctx);
    assert(ctx.state == BOOT_STATE_RELEASED);
    assert(mock_release_buffer_calls == 1);
    assert(mock_destroy_surface_calls == 1);
    assert(mock_release_lease_calls == 1);
    assert(ctx.mapped_pixels == NULL);

    printf("test_snapshot_progress_monotonicity passed\n");
}

static void test_error_event_triggers_recovery(void) {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    boot_display_init(&ctx);

    bh_boot_event_record_t err_rec = {
        .stage = BH_BOOT_STAGE_DRIVERS,
        .status = BH_BOOT_STATUS_ERROR,
        .error_code = -5,
        .component = "VIRTIO",
        .message = "Failed to initialize driver"
    };

    boot_display_handle_record(&ctx, &err_rec);
    assert(ctx.state == BOOT_STATE_RECOVERY);

    boot_display_handoff(&ctx);
    printf("test_error_event_triggers_recovery passed\n");
}

static void test_empty_snapshot_no_advance(void) {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    boot_display_init(&ctx);

    bh_boot_event_snapshot_t empty_snap = {0};
    boot_display_update_from_snapshot(&ctx, &empty_snap);
    assert(ctx.progress_percent == 0);
    assert(ctx.state == BOOT_STATE_SPLASH_ACTIVE);

    boot_display_handoff(&ctx);
    printf("test_empty_snapshot_no_advance passed\n");
}

int main(void) {
    test_successful_init();
    test_failed_open();
    test_unsupported_format();
    test_failed_surface();
    test_stage_progress_mapping();
    test_snapshot_progress_monotonicity();
    test_error_event_triggers_recovery();
    test_empty_snapshot_no_advance();
    printf("All boot_displayd tests passed!\n");
    return 0;
}
