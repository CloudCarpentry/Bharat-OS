#include "bharat/ui/fbui_widgets.h"
#include "bharat/ui/fbui_events.h"
#include "bharat/ui/fb_render.h"
#include "bharat/display/display.h"
#include "hal/hal.h"

// Named design constants
#define BH_UI_NAVY       0xFF0A1128U
#define BH_UI_SAFFRON    0xFFFF671FU
#define BH_UI_GREEN      0xFF046A38U
#define BH_UI_WHITE      0xFFFFFFFFU
#define BH_UI_DARK_BLUE  0xFF101C42U
#define BH_UI_CARD_BG    0xFF12204FU
#define BH_UI_LIGHT_GRAY 0xFFBBBBBBU
#define BH_UI_BORDER     0xFF1D2F6FU

typedef struct {
    bool checked;
} fbui_checkbox_data_t;

static bool g_demo_pipeline_pass = false;

// Custom button event handler for the Run Demo button to capture the click cleanly
extern const fbui_widget_ops_t button_ops; // Use standard button drawing

static bool run_button_handle_event(fbui_widget_t *w, const fbui_event_t *ev) {
    if (!w->visible) return false;

    if (ev->type == FBUI_EVENT_TOUCH_DOWN && fbui_widget_hit_test(w, ev->x, ev->y)) {
        w->focused = true;
        return true;
    } else if (ev->type == FBUI_EVENT_TOUCH_UP) {
        if (w->focused && fbui_widget_hit_test(w, ev->x, ev->y)) {
            g_demo_pipeline_pass = true;
        }
        w->focused = false;
        return true;
    }
    return false;
}

static fbui_widget_ops_t g_run_button_ops;

// A dummy framebuffer for testing when no real hardware exists
// 640x480 ARGB8888 (We upgrade the dummy fb to a clean standard resolution)
#define DUMMY_FB_WIDTH 640
#define DUMMY_FB_HEIGHT 480
static uint32_t dummy_vram[DUMMY_FB_WIDTH * DUMMY_FB_HEIGHT];

static bharat_display_device_t dummy_display = {
    .name = "Dummy offscreen Display",
    .id = 99,
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

void bharat_demo_app(void) {
    hal_serial_write("  [APP] Starting Bharat-OS Live System Dashboard...\n");

    // 1. Get the display device
    bharat_display_device_t *dev = bharat_display_get_default();
    bool is_real = true;

    if (!dev || !dev->framebuffer_base) {
        hal_serial_write("  [APP] No registered real display found.\n");
#if BHARAT_SHOWCASE_GUI
        // On a dedicated showcase target, failure to get a real display is a failure
        hal_serial_write("BHARAT_GUI_DEMO:START\n");
        hal_serial_write("BHARAT_GUI_DEMO:DISPLAY=UNAVAILABLE\n");
        hal_serial_write("BHARAT_GUI_DEMO:ABORT\n");
        return;
#else
        // Fallback for generic/test runs
        hal_serial_write("  [APP] Using offscreen dummy memory framebuffer.\n");
        dev = &dummy_display;
        is_real = false;
#endif
    }

    hal_serial_write("BHARAT_GUI_DEMO:START\n");
    if (is_real) {
        hal_serial_write("BHARAT_GUI_DEMO:DISPLAY=REAL\n");
    } else {
        hal_serial_write("BHARAT_GUI_DEMO:DISPLAY=OFFSCREEN\n");
    }

    // 2. Resolve dimensions dynamically
    uint32_t W = dev->current_mode.width;
    uint32_t H = dev->current_mode.height;

    // Minimum layout bounds checking
    if (W < 320 || H < 240) {
        hal_serial_write("  [APP] Framebuffer too small for graphical dashboard.\n");
        hal_serial_write("BHARAT_GUI_DEMO:ABORT\n");
        return;
    }

    // 3. Initialize the render context
    fbui_render_context_t ctx;
    fbui_render_init(&ctx, dev);
    ctx.background_color = BH_UI_NAVY;

    // Clear the screen with deep navy blue
    fbui_render_fill_rect(&ctx, 0, 0, W, H, BH_UI_NAVY);

    // Draw saffron accent top bar
    fbui_render_fill_rect(&ctx, 0, 0, W, 4, BH_UI_SAFFRON);

    // 4. Build the UI Tree using proportional layout
    hal_serial_write("  [APP] Constructing Dashboard widgets...\n");

    // Title & Header (Row 1)
    fbui_widget_t *lbl_title = fbui_create_label(20, 15, 200, 30, " BHARAT-OS");
    if (lbl_title) {
        lbl_title->fg_color = BH_UI_SAFFRON;
    }

    fbui_widget_t *lbl_status = fbui_create_label((int)W - 160, 15, 140, 30, "SYSTEM ONLINE");
    if (lbl_status) {
        lbl_status->fg_color = BH_UI_GREEN;
    }

    fbui_widget_t *lbl_tagline = fbui_create_label(20, 45, (int)W - 40, 20,
        "Capability-Secure * Composable * Edge Operating Platform");
    if (lbl_tagline) {
        lbl_tagline->fg_color = BH_UI_LIGHT_GRAY;
    }

    // Two-Column Grid coordinates
    int col_w = ((int)W - 60) / 2;
    int col1_x = 20;
    int col2_x = col1_x + col_w + 20;
    int grid_y = 80;
    int grid_h = 130;

    // Background Cards for columns
    fbui_render_fill_rect(&ctx, (uint32_t)col1_x, (uint32_t)grid_y, (uint32_t)col_w, (uint32_t)grid_h, BH_UI_CARD_BG);
    fbui_render_fill_rect(&ctx, (uint32_t)col2_x, (uint32_t)grid_y, (uint32_t)col_w, (uint32_t)grid_h, BH_UI_CARD_BG);

    // Left Column: SYSTEM
    fbui_widget_t *lbl_sys_hdr = fbui_create_label(col1_x + 10, grid_y + 10, col_w - 20, 20, "SYSTEM");
    if (lbl_sys_hdr) lbl_sys_hdr->fg_color = BH_UI_SAFFRON;

    fbui_widget_t *lbl_sys_arch = fbui_create_label(col1_x + 15, grid_y + 35, col_w - 30, 20, "Architecture   x86_64");
    fbui_widget_t *lbl_sys_prof = fbui_create_label(col1_x + 15, grid_y + 55, col_w - 30, 20, "Profile        Desktop");
    fbui_widget_t *lbl_sys_exec = fbui_create_label(col1_x + 15, grid_y + 75, col_w - 30, 20, "Execution      GP");
    fbui_widget_t *lbl_sys_pers = fbui_create_label(col1_x + 15, grid_y + 95, col_w - 30, 20, "Personality    Native");

    // Right Column: PLATFORM
    fbui_widget_t *lbl_plat_hdr = fbui_create_label(col2_x + 10, grid_y + 10, col_w - 20, 20, "PLATFORM");
    if (lbl_plat_hdr) lbl_plat_hdr->fg_color = BH_UI_SAFFRON;

    fbui_widget_t *lbl_plat_sched = fbui_create_label(col2_x + 15, grid_y + 35, col_w - 30, 20, "CPU / Scheduler       READY");
    fbui_widget_t *lbl_plat_ipc   = fbui_create_label(col2_x + 15, grid_y + 55, col_w - 30, 20, "IPC                    READY");
    fbui_widget_t *lbl_plat_disp  = fbui_create_label(col2_x + 15, grid_y + 75, col_w - 30, 20, "Display / FBUI         ACTIVE");
    fbui_widget_t *lbl_plat_cap   = fbui_create_label(col2_x + 15, grid_y + 95, col_w - 30, 20, "Capability Security    ENABLED");

    // Progress Section (Row 3)
    int prog_y = grid_y + grid_h + 15;
    fbui_widget_t *lbl_prog_hdr = fbui_create_label(20, prog_y, (int)W - 40, 20, "BOOT PROGRESS");
    if (lbl_prog_hdr) lbl_prog_hdr->fg_color = BH_UI_SAFFRON;

    fbui_widget_t *prog_bar = fbui_create_progress(20, prog_y + 20, (int)W - 100, 20, 1.0f);
    fbui_widget_t *lbl_prog_pct = fbui_create_label((int)W - 70, prog_y + 20, 50, 20, "100%");

    // Button Section (Row 4)
    int btn_y = prog_y + 55;
    int btn_w = 120;
    int btn_gap = 15;

    fbui_widget_t *btn_info = fbui_create_button(20, btn_y, btn_w, 35, "System Info");
    fbui_widget_t *btn_run  = fbui_create_button(20 + btn_w + btn_gap, btn_y, btn_w, 35, "Run Demo");
    fbui_widget_t *btn_diag = fbui_create_button(20 + (btn_w + btn_gap) * 2, btn_y, btn_w, 35, "Diagnostics");

    // Configure Run Button custom ops for synthetic click event
    if (btn_run) {
        g_run_button_ops.draw = button_ops.draw;
        g_run_button_ops.handle_event = run_button_handle_event;
        btn_run->ops = &g_run_button_ops;
        btn_run->border_color = BH_UI_SAFFRON;
        btn_run->fg_color = BH_UI_WHITE;
        btn_run->bg_color = BH_UI_DARK_BLUE;
    }

    // Status / Checkbox section (Row 5)
    int chk_y = btn_y + 50;
    int chk_w = 140;
    fbui_widget_t *chk_fb      = fbui_create_checkbox(20, chk_y, chk_w, 20, true);
    fbui_widget_t *chk_widgets = fbui_create_checkbox(20 + chk_w, chk_y, chk_w, 20, true);
    fbui_widget_t *chk_events  = fbui_create_checkbox(20 + chk_w * 2, chk_y, chk_w, 20, false);

    if (chk_fb) fbui_widget_set_text(chk_fb, "Framebuffer");
    if (chk_widgets) fbui_widget_set_text(chk_widgets, "Widgets");
    if (chk_events) fbui_widget_set_text(chk_events, "Event Dispatch");

    // Diagnostic/Pipeline text
    fbui_widget_t *lbl_pipe_state = fbui_create_label(20, chk_y + 25, 250, 20, "Event Pipeline: READY");

    // Footer
    fbui_widget_t *lbl_footer = fbui_create_label(20, (int)H - 30, (int)W - 40, 20,
        "Bharat-OS Developer Preview * QEMU x86_64");
    if (lbl_footer) {
        lbl_footer->fg_color = BH_UI_LIGHT_GRAY;
    }

    // Chain all widgets into a list for easy recursive rendering and event handling
    if (lbl_title) lbl_title->next = lbl_status;
    if (lbl_status) lbl_status->next = lbl_tagline;
    if (lbl_tagline) lbl_tagline->next = lbl_sys_hdr;
    if (lbl_sys_hdr) lbl_sys_hdr->next = lbl_sys_arch;
    if (lbl_sys_arch) lbl_sys_arch->next = lbl_sys_prof;
    if (lbl_sys_prof) lbl_sys_prof->next = lbl_sys_exec;
    if (lbl_sys_exec) lbl_sys_exec->next = lbl_sys_pers;
    if (lbl_sys_pers) lbl_sys_pers->next = lbl_plat_hdr;
    if (lbl_plat_hdr) lbl_plat_hdr->next = lbl_plat_sched;
    if (lbl_plat_sched) lbl_plat_sched->next = lbl_plat_ipc;
    if (lbl_plat_ipc) lbl_plat_ipc->next = lbl_plat_disp;
    if (lbl_plat_disp) lbl_plat_disp->next = lbl_plat_cap;
    if (lbl_plat_cap) lbl_plat_cap->next = lbl_prog_hdr;
    if (lbl_prog_hdr) lbl_prog_hdr->next = prog_bar;
    if (prog_bar) prog_bar->next = lbl_prog_pct;
    if (lbl_prog_pct) lbl_prog_pct->next = btn_info;
    if (btn_info) btn_info->next = btn_run;
    if (btn_run) btn_run->next = btn_diag;
    if (btn_diag) btn_diag->next = chk_fb;
    if (chk_fb) chk_fb->next = chk_widgets;
    if (chk_widgets) chk_widgets->next = chk_events;
    if (chk_events) chk_events->next = lbl_pipe_state;
    if (lbl_pipe_state) lbl_pipe_state->next = lbl_footer;

    // 5. Initial Render Pass
    hal_serial_write("  [APP] Initial render of dashboard...\n");
    fbui_widget_t *curr = lbl_title;
    while (curr) {
        if (curr->ops && curr->ops->draw) {
            curr->ops->draw(curr, &ctx);
        }
        curr = curr->next;
    }

    hal_serial_write("BHARAT_GUI_DEMO:RENDER=PASS\n");

    // 6. Setup Event Loop & Dispatch exactly one synthetic event
    fbui_event_loop_t ev_loop;
    fbui_event_loop_init(&ev_loop, lbl_title);

    if (btn_run) {
        fbui_event_t ev_press;
        ev_press.type = FBUI_EVENT_TOUCH_DOWN;
        ev_press.x = btn_run->x + btn_run->width / 2;
        ev_press.y = btn_run->y + btn_run->height / 2;
        ev_press.keycode = 0;

        hal_serial_write("  [APP] Dispatching synthetic touch DOWN on [Run Demo]...\n");
        fbui_dispatch_event(&ev_loop, &ev_press);

        fbui_event_t ev_release;
        ev_release.type = FBUI_EVENT_TOUCH_UP;
        ev_release.x = btn_run->x + btn_run->width / 2;
        ev_release.y = btn_run->y + btn_run->height / 2;
        ev_release.keycode = 0;

        hal_serial_write("  [APP] Dispatching synthetic touch UP on [Run Demo]...\n");
        fbui_dispatch_event(&ev_loop, &ev_release);
    }

    // 7. Process state change and update widgets
    if (g_demo_pipeline_pass) {
        hal_serial_write("  [APP] State update: [Event Dispatch] checked, Pipeline passed.\n");

        // Update checkbox state
        if (chk_events) {
            fbui_checkbox_data_t *cdata = (fbui_checkbox_data_t *)chk_events->priv_data;
            if (cdata) cdata->checked = true;
        }

        // Update pipeline state text
        if (lbl_pipe_state) {
            fbui_widget_set_text(lbl_pipe_state, "Event Pipeline: PASS");
        }

        // Refresh modified/entire widget tree to reflect changes on screen
        curr = lbl_title;
        while (curr) {
            if (curr->ops && curr->ops->draw) {
                curr->ops->draw(curr, &ctx);
            }
            curr = curr->next;
        }

        hal_serial_write("BHARAT_GUI_DEMO:EVENT=PASS\n");
        if (is_real) {
            hal_serial_write("BHARAT_GUI_DEMO:COMPLETE\n");
        }
    } else {
        hal_serial_write("  [APP] FAIL: Synthetic interaction was not captured.\n");
    }

    hal_serial_write("  [APP] Showcase dashboard completed.\n");
}
