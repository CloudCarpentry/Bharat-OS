#include "bharat/ui/fbui_widgets.h"
#include "bharat/ui/fbui_events.h"
#include "bharat/ui/fb_render.h"
#include "bharat/display/display.h"
#include "hal/hal.h"

// Define a placeholder missing from standard libraries in this minimal env
#ifndef NULL
#define NULL ((void*)0)
#endif

// A dummy framebuffer for testing when no real hardware exists
// 320x240 ARGB8888
#define DUMMY_FB_WIDTH 320
#define DUMMY_FB_HEIGHT 240
static uint32_t dummy_vram[DUMMY_FB_WIDTH * DUMMY_FB_HEIGHT];

static bharat_display_mode_t dummy_mode = {
    .width = DUMMY_FB_WIDTH,
    .height = DUMMY_FB_HEIGHT,
    .stride = DUMMY_FB_WIDTH * 4,
    .bpp = 32,
    .format = BHARAT_PIXEL_FORMAT_ARGB8888,
    .refresh_rate = 60
};

static bharat_display_device_t dummy_display = {
    .name = "Dummy Display",
    .id = 0,
    .framebuffer_base = dummy_vram,
    .framebuffer_size = sizeof(dummy_vram),
    .current_mode = {
        .width = DUMMY_FB_WIDTH,
        .height = DUMMY_FB_HEIGHT,
        .stride = DUMMY_FB_WIDTH * 4,
        .format = BHARAT_PIXEL_FORMAT_ARGB8888
    },
    .ops = NULL
};

// Entry point called by kernel_main
void bharat_demo_app(void) {
    hal_serial_write("  [APP] Starting Bharat-OS Embedded UI Demo App...\n");

    // 1. Get the display device
    bharat_display_device_t *dev = bharat_display_get_default();
    if (!dev || !dev->framebuffer_base) {
        hal_serial_write("  [APP] No real display found. Using dummy memory framebuffer.\n");
        dev = &dummy_display;
    }

    // 2. Initialize the render context
    fbui_render_context_t ctx;
    fbui_render_init(&ctx, dev);
    ctx.background_color = 0xFF222222; // Dark gray background

    // Clear the screen
    fbui_render_fill_rect(&ctx, 0, 0, dev->current_mode.width, dev->current_mode.height, ctx.background_color);

    // 3. Build the UI Tree
    hal_serial_write("  [APP] Building Widget Tree...\n");

    // Root Panel (we'll just use a linked list of widgets for this simple demo)
    fbui_widget_t *label1 = fbui_create_label(10, 10, 200, 30, "System Settings");
    if (label1) {
        label1->bg_color = 0x00000000; // Transparent-ish
        label1->fg_color = 0xFFFFFFFF; // White text
    }

    fbui_widget_t *btn1 = fbui_create_button(10, 50, 120, 40, "Enable Wi-Fi");
    fbui_widget_t *prog1 = fbui_create_progress(10, 100, 200, 20, 0.65f);
    fbui_widget_t *slider1 = fbui_create_slider(10, 140, 200, 30, 0.4f);
    fbui_widget_t *chk1 = fbui_create_checkbox(10, 190, 30, 30, true);

    // Chain them together
    if (label1) label1->next = btn1;
    if (btn1) btn1->next = prog1;
    if (prog1) prog1->next = slider1;
    if (slider1) slider1->next = chk1;

    // 4. Render the UI
    hal_serial_write("  [APP] Rendering UI onto Framebuffer...\n");
    fbui_widget_t *current = label1;
    while (current) {
        if (current->ops && current->ops->draw) {
            current->ops->draw(current, &ctx);
        }
        current = current->next;
    }

    // 5. Simulate an Event (Optional, just to show the Event Loop API works)
    fbui_event_loop_t ev_loop;
    fbui_event_loop_init(&ev_loop, label1);

    fbui_event_t touch_ev;
    touch_ev.type = FBUI_EVENT_TOUCH_DOWN;
    touch_ev.x = 20;
    touch_ev.y = 200; // Hits the checkbox
    touch_ev.keycode = 0;

    hal_serial_write("  [APP] Dispatching synthetic touch event...\n");
    fbui_dispatch_event(&ev_loop, &touch_ev);

    touch_ev.type = FBUI_EVENT_TOUCH_UP;
    fbui_dispatch_event(&ev_loop, &touch_ev);

    // Re-render to show updated checkbox state
    current = label1;
    while (current) {
        if (current->ops && current->ops->draw) {
            current->ops->draw(current, &ctx);
        }
        current = current->next;
    }

    hal_serial_write("  [APP] Demo App completed successfully.\n");
}
