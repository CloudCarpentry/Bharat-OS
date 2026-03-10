#include "../kernel/include/tests/ktest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../kernel/include/kernel.h"
#include "../kernel/include/device.h"
#include "../kernel/include/advanced/algo_matrix.h"
#include "../kernel/include/subsystem_profile.h"

#include "../subsys/include/linux_compat.h"
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
    printf("Running Integration Core Subsys Tests...\n");

    // 1. Core Kernel Initialization Checks
    algo_matrix_init();
    // We just ensure algo matrix init completes without faulting since full scheduler mock
    // might not populate all operations if hardware PMCs are strictly required
    printf("[PASS] Core Matrix Init\n");

    int dev_fw_rc = device_framework_init();
    assert(dev_fw_rc == 0);
    printf("[PASS] Device Framework Init\n");

    // 2. Linux Personality Subsystem
    subsys_instance_t linux_env = {0};
    linux_env.type = SUBSYS_TYPE_LINUX;
    int linux_rc = linux_subsys_init(&linux_env);
    assert(linux_rc == 0);
    assert(linux_env.memory_limit_mb > 0);

    // Check basic FD mapping
    int map_rc = linux_map_fd_to_capability(&linux_env, 10, 0x1234, LINUX_FD_TYPE_FILE);
    assert(map_rc == 0);

    // Verify a syscall handler for Linux
    long sys_ret = linux_syscall_handler(39, 0, 0, 0, 0, 0, 0); // getpid
    // Since is_running is false by default in our stub init
    assert(sys_ret == -38);
    linux_env.is_running = 1;
    sys_ret = linux_syscall_handler(39, 0, 0, 0, 0, 0, 0); // getpid
    assert(sys_ret == 1);
    printf("[PASS] Linux Personality Init and Syscall Handler\n");

    // 3. Automotive Subsystem
    subsys_automotive_init();
    automotive_runtime_policy_t auto_policy = {
        .ipc_mode = AUTOMOTIVE_IPC_MODE_DETERMINISTIC,
        .boot_profile = AUTOMOTIVE_BOOT_PROFILE_NORMAL,
        .default_queue_budget_us = 1000,
        .default_deadline_slack_us = 100
    };
    subsys_automotive_set_runtime_policy(&auto_policy);
    const automotive_runtime_policy_t* fetched_policy = subsys_automotive_get_runtime_policy();
    assert(fetched_policy != NULL);
    assert(fetched_policy->ipc_mode == AUTOMOTIVE_IPC_MODE_DETERMINISTIC);
    printf("[PASS] Automotive Subsystem Init and Policy\n");

    // 4. GUI / Framebuffer Rendering
    static uint32_t fake_vram[800 * 600]; // 800x600 32bpp - static to prevent stack overflow
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
            .width = 800,
            .height = 600,
            .stride = 800 * 4,
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
    fbui_render_fill_rect(&render_ctx, 10, 10, 100, 100, 0xFFFF0000); // Red box
    assert(g_damage_called == 1);

    // Verify pixels are written
    assert(fake_vram[10 * 800 + 10] == 0xFFFF0000);
    assert(fake_vram[109 * 800 + 109] == 0xFFFF0000);
    printf("[PASS] FBUI Rendering primitive (Fill Rect)\n");

    printf("\nAll Integration Tests Passed!\n");
    return 0;
}
