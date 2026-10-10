#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bharat/uapi/display/display_v2.h"
#include "../display_client.h"

// Forward declarations of exposed internals
typedef enum {
    BOOT_STATE_UNAVAILABLE,
    BOOT_STATE_INITIALIZING,
    BOOT_STATE_SPLASH_ACTIVE,
    BOOT_STATE_RECOVERY,
    BOOT_STATE_HANDOFF_PENDING,
    BOOT_STATE_RELEASED,
    BOOT_STATE_FAILED
} boot_display_state_t;

typedef enum {
    BH_BOOT_EVENT_KERNEL_READY,
    BH_BOOT_EVENT_INIT_STARTED,
    BH_BOOT_EVENT_SERVICE_SPAWNED,
    BH_BOOT_EVENT_SERVICE_READY,
    BH_BOOT_EVENT_SERVICE_FAILED,
    BH_BOOT_EVENT_DISPLAY_READY,
    BH_BOOT_EVENT_HANDOFF_COMPLETE
} bh_boot_event_t;

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
extern void boot_display_handle_event(boot_display_ctx_t *ctx, bh_boot_event_t event);

// Mock implementation configuration
static bh_display_result_t mock_open_result = BH_DISPLAY_RESULT_OK;
static uint32_t mock_pixel_format = BH_DISPLAY_FORMAT_XRGB8888;
static bh_display_result_t mock_surface_result = BH_DISPLAY_RESULT_OK;
static bh_display_result_t mock_register_result = BH_DISPLAY_RESULT_OK;
static bh_display_result_t mock_attach_result = BH_DISPLAY_RESULT_OK;


// Mocks for display_client.h
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
    if (mock_surface_result == BH_DISPLAY_RESULT_OK) {
        *out_surface = 3;
    }
    return mock_surface_result;
}

bh_display_result_t bh_client_register_buffer(bh_display_lease_handle_t lease, bh_display_buffer_desc_t *desc, bh_gui_buffer_handle_t *out_buffer, void **out_mapped) {
    if (mock_register_result == BH_DISPLAY_RESULT_OK) {
        *out_buffer = 4;
        *out_mapped = malloc(800 * 600 * 4);
    }
    return mock_register_result;
}

bh_display_result_t bh_client_attach_buffer(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, bh_gui_buffer_handle_t buffer) {
    return mock_attach_result;
}

bh_display_result_t bh_client_present_surface(bh_display_lease_handle_t lease, bh_gui_surface_handle_t surface, bh_gui_buffer_handle_t buffer, bh_gui_fence_handle_t *out_release_fence) {
    *out_release_fence = BH_GUI_HANDLE_INVALID;
    return BH_DISPLAY_RESULT_OK;
}

void bharat_runtime_log(const char *msg) {
    // printf("LOG: %s\n", msg);
}

void bharat_tiny_ui_init(void *state, bool safe_mode) {}
void bharat_tiny_ui_render(const void *fb, const void *state) {}

void reset_mocks() {
    mock_open_result = BH_DISPLAY_RESULT_OK;
    mock_pixel_format = BH_DISPLAY_FORMAT_XRGB8888;
    mock_surface_result = BH_DISPLAY_RESULT_OK;
    mock_register_result = BH_DISPLAY_RESULT_OK;
    mock_attach_result = BH_DISPLAY_RESULT_OK;
}

void test_successful_init() {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == 0);
    assert(ctx.state == BOOT_STATE_SPLASH_ACTIVE);
    assert(ctx.fb.width_px == 800);
    assert(ctx.fb.height_px == 600);
    printf("test_successful_init passed\n");
}

void test_failed_open() {
    reset_mocks();
    mock_open_result = BH_DISPLAY_RESULT_NOT_FOUND;
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == -1);
    assert(ctx.state == BOOT_STATE_UNAVAILABLE);
    printf("test_failed_open passed\n");
}

void test_unsupported_format() {
    reset_mocks();
    mock_pixel_format = BH_DISPLAY_FORMAT_RGB565; // Unsupported format
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == -1);
    assert(ctx.state == BOOT_STATE_FAILED);
    printf("test_unsupported_format passed\n");
}

void test_failed_surface() {
    reset_mocks();
    mock_surface_result = BH_DISPLAY_RESULT_NO_RESOURCES;
    boot_display_ctx_t ctx = {0};
    int res = boot_display_init(&ctx);
    assert(res == -1);
    assert(ctx.state == BOOT_STATE_FAILED);
    printf("test_failed_surface passed\n");
}

void test_handoff_sequence() {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    boot_display_init(&ctx);

    assert(ctx.state == BOOT_STATE_SPLASH_ACTIVE);

    boot_display_handle_event(&ctx, BH_BOOT_EVENT_KERNEL_READY);
    assert(ctx.progress_percent == 10);

    boot_display_handle_event(&ctx, BH_BOOT_EVENT_INIT_STARTED);
    assert(ctx.progress_percent == 20);

    boot_display_handle_event(&ctx, BH_BOOT_EVENT_DISPLAY_READY);
    assert(ctx.state == BOOT_STATE_HANDOFF_PENDING);
    assert(ctx.progress_percent == 100);

    boot_display_handle_event(&ctx, BH_BOOT_EVENT_HANDOFF_COMPLETE);
    assert(ctx.state == BOOT_STATE_RELEASED);
    assert(ctx.mapped_pixels == NULL);

    printf("test_handoff_sequence passed\n");
}

void test_service_failure_recovery() {
    reset_mocks();
    boot_display_ctx_t ctx = {0};
    boot_display_init(&ctx);

    boot_display_handle_event(&ctx, BH_BOOT_EVENT_INIT_STARTED);
    assert(ctx.state == BOOT_STATE_SPLASH_ACTIVE);

    boot_display_handle_event(&ctx, BH_BOOT_EVENT_SERVICE_FAILED);
    assert(ctx.state == BOOT_STATE_RECOVERY);

    printf("test_service_failure_recovery passed\n");
}

int main(void) {
    test_successful_init();
    test_failed_open();
    test_unsupported_format();
    test_failed_surface();
    test_handoff_sequence();
    test_service_failure_recovery();
    printf("All tests passed!\n");
    return 0;
}
