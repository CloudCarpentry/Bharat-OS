#include "../kernel/include/tests/ktest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../kernel/include/kernel.h"
#include "../kernel/include/device.h"
#include "../kernel/include/advanced/algo_matrix.h"
#include "../kernel/include/subsystem_profile.h"

#include "../subsys/include/bharat/automotive/automotive.h"

#include "../include/bharat/ui/fb_render.h"

// Stubs for testing
static int stub_display_enable(struct bharat_display_device *dev) { (void)dev; return 0; }
static int stub_display_disable(struct bharat_display_device *dev) { (void)dev; return 0; }
static int stub_display_set_mode(struct bharat_display_device *dev, const bharat_display_mode_t *mode) {
    if (dev && mode) dev->current_mode = *mode;
    return 0;
}

static int g_damage_called = 0;
int bharat_display_update_damage(bharat_display_device_t *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    (void)dev; (void)x; (void)y; (void)w; (void)h;
    g_damage_called = 1;
    return 0;
}

int main(void) {
    printf("Running Edge/Embedded Profile Tests...\n");

    // 1. Core Kernel Initialization Checks (same as full)
    algo_matrix_init();
    printf("[PASS] Core Matrix Init\n");

    int dev_fw_rc = device_framework_init();
    assert(dev_fw_rc == 0);
    printf("[PASS] Device Framework Init\n");

    // Note: No Linux personality checked here. This simulates Edge profile.

    // 2. Automotive Subsystem (often used in Edge)
    subsys_automotive_init();
    automotive_runtime_policy_t auto_policy = {
        .ipc_mode = AUTOMOTIVE_IPC_MODE_DETERMINISTIC,
        .boot_profile = AUTOMOTIVE_BOOT_PROFILE_RT_MINIMAL,
        .default_queue_budget_us = 500, // Faster budget for edge
        .default_deadline_slack_us = 50
    };
    subsys_automotive_set_runtime_policy(&auto_policy);
    const automotive_runtime_policy_t* fetched_policy = subsys_automotive_get_runtime_policy();
    assert(fetched_policy != NULL);
    assert(fetched_policy->ipc_mode == AUTOMOTIVE_IPC_MODE_DETERMINISTIC);
    printf("[PASS] Automotive Subsystem Init (Edge Policy)\n");

    // 3. GUI / Framebuffer Rendering (common in embedded displays)
    static uint32_t fake_vram[320 * 240]; // 320x240 32bpp for embedded - static to prevent stack overflow
    bharat_display_device_ops_t display_ops = {
        .enable = stub_display_enable,
        .disable = stub_display_disable,
        .set_mode = stub_display_set_mode,
        .get_mode = NULL,
        .flush = NULL,
        .set_backlight = NULL,
        .mmap = NULL
    };
    bharat_display_device_t display = {
        .name = "stub_display",
        .id = 1,
        .framebuffer_base = fake_vram,
        .framebuffer_size = sizeof(fake_vram),
        .current_mode = {
            .width = 320,
            .height = 240,
            .stride = 320 * 4,
            .bpp = 32,
            .format = BHARAT_PIXEL_FORMAT_ARGB8888,
            .refresh_rate = 60
        },
        .ops = &display_ops
    };

    fbui_render_context_t render_ctx;
    fbui_render_init(&render_ctx, &display);
    assert(render_ctx.device == &display);

    g_damage_called = 0;
    fbui_render_draw_line(&render_ctx, 0, 0, 100, 100, 0xFF00FF00); // Green line
    assert(g_damage_called == 1);

    // Check line starting pixel
    assert(fake_vram[0] == 0xFF00FF00);
    assert(fake_vram[100 * 320 + 100] == 0xFF00FF00);
    printf("[PASS] FBUI Rendering primitive (Draw Line)\n");

    printf("\nEdge Profile Tests Passed!\n");
    return 0;
}
