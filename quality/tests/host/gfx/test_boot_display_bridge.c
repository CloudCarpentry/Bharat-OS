#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "bharat/display/display.h"
#include "bharat/display/boot_video.h"

// Mock functions required by boot_display_bridge.c
static int g_mock_register_count = 0;
static bharat_display_device_t g_last_registered_dev;

int bharat_display_register(bharat_display_device_t *dev) {
    g_mock_register_count++;
    g_last_registered_dev = *dev;
    return 0; // Success
}

void hal_serial_write(const char *str) {
    printf("[SERIAL MOCK] %s", str);
}

// Include the C file directly to test internal static helper functions
#include "core/kernel/src/display/boot_display_bridge.c"

int main(void) {
    printf("Running test_boot_display_bridge...\n");

    // Test 1: format conversion
    bharat_pixel_format_t out_fmt;
    bool ok = boot_format_to_display_format(PIXEL_FORMAT_ARGB8888, &out_fmt);
    assert(ok && out_fmt == BHARAT_PIXEL_FORMAT_ARGB8888);

    ok = boot_format_to_display_format(PIXEL_FORMAT_XRGB8888, &out_fmt);
    assert(ok && out_fmt == BHARAT_PIXEL_FORMAT_XRGB8888);

    ok = boot_format_to_display_format(PIXEL_FORMAT_RGB565, &out_fmt);
    assert(ok && out_fmt == BHARAT_PIXEL_FORMAT_RGB565);

    // Unsupported format (BGRX / unknown)
    ok = boot_format_to_display_format(PIXEL_FORMAT_BGRX8888, &out_fmt);
    assert(!ok);

    // Test 2: validation bounds and overflow protection
    boot_video_handoff_t handoff = {0};
    handoff.valid = true;
    handoff.width = 0; // Invalid
    handoff.height = 480;
    handoff.stride_bytes = 480 * 4;
    handoff.size = 480 * 480 * 4;
    handoff.virt_addr = 0x1000;
    handoff.format = PIXEL_FORMAT_ARGB8888;

    int ret = boot_display_register_from_handoff(&handoff);
    assert(ret == -2); // zero geometry check

    handoff.width = 640;
    handoff.stride_bytes = 640 * 4;
    // Overflow check
    handoff.height = 0xFFFFFFFF; // extremely large height
    ret = boot_display_register_from_handoff(&handoff);
    assert(ret == -3); // multiplication overflow check

    // Small buffer size check
    handoff.height = 480;
    handoff.size = 100; // smaller than height * stride
    ret = boot_display_register_from_handoff(&handoff);
    assert(ret == -4); // size check

    // Correct handoff check
    handoff.size = 640 * 480 * 4;
    ret = boot_display_register_from_handoff(&handoff);
    assert(ret == 0); // Success!
    assert(g_mock_register_count == 1);
    assert(g_last_registered_dev.current_mode.width == 640);
    assert(g_last_registered_dev.current_mode.height == 480);
    assert(g_last_registered_dev.current_mode.format == BHARAT_PIXEL_FORMAT_ARGB8888);

    // Idempotency check
    ret = boot_display_register_from_handoff(&handoff);
    assert(ret == 0);
    assert(g_mock_register_count == 1); // should not re-register

    printf("test_boot_display_bridge passed successfully!\n");
    return 0;
}
