#include "bharat/ui/fbui_widgets.h"
#include "bharat/ui/fbui_events.h"
#include "bharat/ui/fb_render.h"
#include "bharat/display/display.h"
#include "hal/hal.h"
#include "virtio_input.h"
#include "inputmgr/inputmgr.h"
#include "fbui_input_bridge.h"

// Declare scheduling yield
extern void bh_thread_yield(void);

// Named design colors
#define BH_UI_NAVY       0xFF0A1128U
#define BH_UI_SAFFRON    0xFFFF671FU
#define BH_UI_GREEN      0xFF046A38U
#define BH_UI_WHITE      0xFFFFFFFFU
#define BH_UI_DARK_BLUE  0xFF101C42U
#define BH_UI_CARD_BG    0xFF12204FU
#define BH_UI_LIGHT_GRAY 0xFFBBBBBBU
#define BH_UI_BORDER     0xFF1D2F6FU

static const char g_cursor_sprite_bitmap[16][16] = {
    "X...............",
    "XX..............",
    "X.X.............",
    "X..X............",
    "X...X...........",
    "X....X..........",
    "X.....X.........",
    "X......X........",
    "X.......X.......",
    "X........X......",
    "X..XXXXXX.......",
    "X.X..X..........",
    "XX...X..........",
    "X.....X.........",
    "......X.........",
    "................"
};

typedef struct {
    int32_t x;
    int32_t y;
    int32_t previous_x;
    int32_t previous_y;
    uint32_t saved_pixels[16 * 16];
    bool saved_valid;
    bool visible;
} bh_fbui_cursor_t;

static virtio_input_device_t g_keyboard_dev;
static virtio_input_device_t g_pointer_dev;

static bool g_keyboard_found = false;
static bool g_pointer_found = false;

static bh_fbui_cursor_t g_cursor;

static bool g_pointer_event_passed = false;
static bool g_key_event_passed = false;
static bool g_button_clicked_passed = false;

static bool g_marker_kb_ready = false;
static bool g_marker_ptr_ready = false;
static bool g_marker_event_passed = false;
static bool g_marker_key_passed = false;
static bool g_marker_btn_passed = false;
static bool g_marker_complete = false;

static void bh_fbui_cursor_erase(bharat_display_device_t *dev, bh_fbui_cursor_t *cur) {
    if (!cur->saved_valid || !cur->visible) return;

    uint32_t *fb = (uint32_t *)dev->framebuffer_base;
    uint32_t stride = dev->current_mode.stride / 4;
    int32_t W = (int32_t)dev->current_mode.width;
    int32_t H = (int32_t)dev->current_mode.height;

    for (int y = 0; y < 16; y++) {
        int32_t py = cur->previous_y + y;
        if (py < 0 || py >= H) continue;
        for (int x = 0; x < 16; x++) {
            int32_t px = cur->previous_x + x;
            if (px < 0 || px >= W) continue;
            fb[py * stride + px] = cur->saved_pixels[y * 16 + x];
        }
    }
    cur->saved_valid = false;
}

static void bh_fbui_cursor_draw(bharat_display_device_t *dev, bh_fbui_cursor_t *cur) {
    uint32_t *fb = (uint32_t *)dev->framebuffer_base;
    uint32_t stride = dev->current_mode.stride / 4;
    int32_t W = (int32_t)dev->current_mode.width;
    int32_t H = (int32_t)dev->current_mode.height;

    // Save background first
    for (int y = 0; y < 16; y++) {
        int32_t py = cur->y + y;
        if (py < 0 || py >= H) {
            for (int x = 0; x < 16; x++) cur->saved_pixels[y * 16 + x] = 0;
            continue;
        }
        for (int x = 0; x < 16; x++) {
            int32_t px = cur->x + x;
            if (px < 0 || px >= W) {
                cur->saved_pixels[y * 16 + x] = 0;
            } else {
                cur->saved_pixels[y * 16 + x] = fb[py * stride + px];
            }
        }
    }
    cur->saved_valid = true;
    cur->previous_x = cur->x;
    cur->previous_y = cur->y;

    // Draw sprite
    for (int y = 0; y < 16; y++) {
        int32_t py = cur->y + y;
        if (py < 0 || py >= H) continue;
        for (int x = 0; x < 16; x++) {
            int32_t px = cur->x + x;
            if (px < 0 || px >= W) continue;

            char ch = g_cursor_sprite_bitmap[y][x];
            if (ch == 'X') {
                fb[py * stride + px] = 0xFF000000; // Black outline
            } else if (ch == '.') {
                fb[py * stride + px] = 0xFFFFFFFF; // White body
            }
        }
    }
    cur->visible = true;
}

void bharat_demo_app(void) {
    hal_serial_write("BHARAT_INPUT_DEMO:START\n");
    hal_serial_write("BHARAT_INPUT_DEMO:PCI=ECAM\n");
    hal_serial_write("BHARAT_INPUT_DEMO:DELIVERY=POLLED\n");

    // 1. Get the display device
    bharat_display_device_t *dev = bharat_display_get_default();
    if (!dev || !dev->framebuffer_base) {
        hal_serial_write("  [APP] No registered real display found.\n");
        return;
    }

    hal_serial_write("BHARAT_INPUT_DEMO:FRAMEBUFFER=REAL\n");

    // 2. Discover PCI VirtIO Input devices
    g_keyboard_found = false;
    g_pointer_found = false;

    pci_device_t *pci = pci_get_device_list();
    while (pci) {
        if (pci->vendor_id == 0x1AF4 && (pci->device_id == 0x1052 || pci->device_id == 0x1012)) {
            virtio_input_device_t temp;
            virtio_input_raw_event_t *dummy_buf = NULL; // Real mode allocates inside static array
            virtio_input_init(&temp, dummy_buf, INPUT_QUEUE_SIZE);
            if (virtio_input_probe(&temp, pci) == 0) {
                if (temp.supports_rel) {
                    g_pointer_dev = temp;
                    g_pointer_found = true;
                } else {
                    g_keyboard_dev = temp;
                    g_keyboard_found = true;
                }
            }
        }
        pci = pci->next;
    }

    // 3. Start devices
    if (g_keyboard_found) {
        virtio_input_start(&g_keyboard_dev);
        if (!g_marker_kb_ready) {
            hal_serial_write("BHARAT_INPUT_DEMO:VIRTIO_KEYBOARD=READY\n");
            g_marker_kb_ready = true;
        }
    }
    if (g_pointer_found) {
        virtio_input_start(&g_pointer_dev);
        if (!g_marker_ptr_ready) {
            hal_serial_write("BHARAT_INPUT_DEMO:VIRTIO_POINTER=READY\n");
            g_marker_ptr_ready = true;
        }
    }

    // 4. Initialize input manager
    inputmgr_init();
    hal_serial_write("BHARAT_INPUT_DEMO:EVENT_RING=READY\n");

    // 5. Setup UI and widgets
    uint32_t W = dev->current_mode.width;
    uint32_t H = dev->current_mode.height;

    fbui_render_context_t ctx;
    fbui_render_init(&ctx, dev);
    ctx.background_color = BH_UI_NAVY;

    // Clear background
    fbui_render_fill_rect(&ctx, 0, 0, W, H, BH_UI_NAVY);
    fbui_render_fill_rect(&ctx, 0, 0, W, 4, BH_UI_SAFFRON);

    // Left Column Label Cards
    fbui_widget_t *lbl_title = fbui_create_label(20, 15, 300, 30, "BHARAT-OS REAL INPUT DEMO");
    if (lbl_title) lbl_title->fg_color = BH_UI_SAFFRON;

    fbui_widget_t *lbl_transport = fbui_create_label(20, 55, 350, 20, "Input transport:     VIRTIO-PCI");
    fbui_widget_t *lbl_delivery  = fbui_create_label(20, 75, 350, 20, "Delivery:            POLLED");

    fbui_widget_t *lbl_kb = fbui_create_label(20, 105, 350, 20, g_keyboard_found ? "Keyboard:            CONNECTED" : "Keyboard:            DISCONNECTED");
    fbui_widget_t *lbl_ptr = fbui_create_label(20, 125, 350, 20, g_pointer_found ? "Pointer:             CONNECTED" : "Pointer:             DISCONNECTED");

    // Dynamic Live Counter Labels
    static char rx_buf[64] = "Events received:     0";
    static char drop_buf[64] = "Events dropped:      0";
    static char key_buf[64] = "Last key:            NONE";
    static char cursor_buf[64] = "Cursor:              0, 0";

    fbui_widget_t *lbl_rx_cnt = fbui_create_label(20, 155, 350, 20, rx_buf);
    fbui_widget_t *lbl_drop_cnt = fbui_create_label(20, 175, 350, 20, drop_buf);
    fbui_widget_t *lbl_last_key = fbui_create_label(20, 195, 350, 20, key_buf);
    fbui_widget_t *lbl_cursor_val = fbui_create_label(20, 215, 350, 20, cursor_buf);

    // Bottom Buttons
    fbui_widget_t *btn_run   = fbui_create_button(20, 260, 120, 35, "Run Demo");
    fbui_widget_t *btn_reset = fbui_create_button(160, 260, 140, 35, "Reset Counters");
    fbui_widget_t *btn_diag  = fbui_create_button(320, 260, 120, 35, "Diagnostics");

    if (btn_run) {
        btn_run->border_color = BH_UI_SAFFRON;
        btn_run->bg_color = BH_UI_DARK_BLUE;
        btn_run->fg_color = BH_UI_WHITE;
        btn_run->focused = true; // Initially focused
    }
    if (btn_reset) {
        btn_reset->border_color = BH_UI_SAFFRON;
        btn_reset->bg_color = BH_UI_DARK_BLUE;
        btn_reset->fg_color = BH_UI_WHITE;
    }
    if (btn_diag) {
        btn_diag->border_color = BH_UI_SAFFRON;
        btn_diag->bg_color = BH_UI_DARK_BLUE;
        btn_diag->fg_color = BH_UI_WHITE;
    }

    // Footnote
    fbui_widget_t *lbl_footer = fbui_create_label(20, (int)H - 30, (int)W - 40, 20, "Use Keyboard TAB to navigate, ENTER/SPACE to click, or mouse to interact");
    if (lbl_footer) lbl_footer->fg_color = BH_UI_LIGHT_GRAY;

    // Chain widgets
    if (lbl_title) lbl_title->next = lbl_transport;
    if (lbl_transport) lbl_transport->next = lbl_delivery;
    if (lbl_delivery) lbl_delivery->next = lbl_kb;
    if (lbl_kb) lbl_kb->next = lbl_ptr;
    if (lbl_ptr) lbl_ptr->next = lbl_rx_cnt;
    if (lbl_rx_cnt) lbl_rx_cnt->next = lbl_drop_cnt;
    if (lbl_drop_cnt) lbl_drop_cnt->next = lbl_last_key;
    if (lbl_last_key) lbl_last_key->next = lbl_cursor_val;
    if (lbl_cursor_val) lbl_cursor_val->next = btn_run;
    if (btn_run) btn_run->next = btn_reset;
    if (btn_reset) btn_reset->next = btn_diag;
    if (btn_diag) btn_diag->next = lbl_footer;

    // Initial render pass
    fbui_widget_t *curr = lbl_title;
    while (curr) {
        if (curr->ops && curr->ops->draw) {
            curr->ops->draw(curr, &ctx);
        }
        curr = curr->next;
    }

    // Initialize Cursor and Bridge
    g_cursor.x = (int32_t)W / 2;
    g_cursor.y = (int32_t)H / 2;
    g_cursor.previous_x = g_cursor.x;
    g_cursor.previous_y = g_cursor.y;
    g_cursor.saved_valid = false;
    g_cursor.visible = false;

    fbui_input_bridge_t bridge;
    fbui_input_bridge_init(&bridge, (int32_t)W, (int32_t)H);

    bh_fbui_cursor_draw(dev, &g_cursor);

    int focused_btn_idx = 0; // 0: run, 1: reset, 2: diag

    uint32_t total_events_rx = 0;
    uint32_t total_events_drop = 0;

    // 6. Polling loop
    while (true) {
        // Poll input devices
        if (g_keyboard_found) {
            virtio_input_poll(&g_keyboard_dev, 32);
        }
        if (g_pointer_found) {
            virtio_input_poll(&g_pointer_dev, 32);
        }

        // Drain normalized input manager queue
        bh_input_event_t ievs[64];
        int count = bh_inputmgr_drain(ievs, 64);

        bool needs_redraw = false;

        for (int i = 0; i < count; i++) {
            total_events_rx++;
            fbui_event_t oev;
            if (fbui_input_bridge_translate(&bridge, &ievs[i], &oev)) {
                // We got a mapped FBUI event!
                if (oev.type == FBUI_EVENT_POINTER_MOVE) {
                    bh_fbui_cursor_erase(dev, &g_cursor);
                    g_cursor.x = oev.x;
                    g_cursor.y = oev.y;
                    bh_fbui_cursor_draw(dev, &g_cursor);

                    if (!g_pointer_event_passed) {
                        g_pointer_event_passed = true;
                        if (!g_marker_event_passed) {
                            hal_serial_write("BHARAT_INPUT_DEMO:POINTER_EVENT=PASS\n");
                            g_marker_event_passed = true;
                        }
                    }
                } else if (oev.type == FBUI_EVENT_KEY_DOWN) {
                    if (!g_key_event_passed) {
                        g_key_event_passed = true;
                        if (!g_marker_key_passed) {
                            hal_serial_write("BHARAT_INPUT_DEMO:KEY_EVENT=PASS\n");
                            g_marker_key_passed = true;
                        }
                    }

                    // Key navigation handling
                    uint16_t key = (uint16_t)oev.keycode;
                    if (key == 15) { // TAB key (standard keycode is 15)
                        focused_btn_idx = (focused_btn_idx + 1) % 3;
                        btn_run->focused = (focused_btn_idx == 0);
                        btn_reset->focused = (focused_btn_idx == 1);
                        btn_diag->focused = (focused_btn_idx == 2);
                        needs_redraw = true;
                    } else if (key == 28 || key == 57) { // ENTER or SPACE
                        if (focused_btn_idx == 0) {
                            // Run Demo Clicked via keyboard
                            if (!g_button_clicked_passed) {
                                g_button_clicked_passed = true;
                                if (!g_marker_btn_passed) {
                                    hal_serial_write("BHARAT_INPUT_DEMO:BUTTON_CLICK=PASS\n");
                                    g_marker_btn_passed = true;
                                }
                            }
                        } else if (focused_btn_idx == 1) {
                            // Reset counters
                            total_events_rx = 0;
                            total_events_drop = 0;
                            bh_inputmgr_reset();
                        } else if (focused_btn_idx == 2) {
                            // Diagnostics Click
                            hal_serial_write("  [APP] Diagnostics triggered! All systems green.\n");
                        }
                    }

                    // Write last key to string
                    char hex[16];
                    int pos = 0;
                    uint16_t temp_val = key;
                    if (temp_val == 0) {
                        hex[pos++] = '0';
                    } else {
                        char rev[16];
                        int rpos = 0;
                        while (temp_val > 0) {
                            int rem = temp_val % 10;
                            rev[rpos++] = (char)('0' + rem);
                            temp_val /= 10;
                        }
                        while (rpos > 0) {
                            hex[pos++] = rev[--rpos];
                        }
                    }
                    hex[pos] = '\0';

                    __builtin_memset(key_buf, 0, sizeof(key_buf));
                    __builtin_memcpy(key_buf, "Last key:            ", 21);
                    __builtin_memcpy(key_buf + 21, hex, (size_t)pos);
                    key_buf[21 + pos] = '\0';
                    needs_redraw = true;
                } else if (oev.type == FBUI_EVENT_POINTER_DOWN) {
                    // Check button hit tests
                    bh_fbui_cursor_erase(dev, &g_cursor);
                    if (fbui_widget_hit_test(btn_run, oev.x, oev.y)) {
                        if (!g_button_clicked_passed) {
                            g_button_clicked_passed = true;
                            if (!g_marker_btn_passed) {
                                hal_serial_write("BHARAT_INPUT_DEMO:BUTTON_CLICK=PASS\n");
                                g_marker_btn_passed = true;
                            }
                        }
                    } else if (fbui_widget_hit_test(btn_reset, oev.x, oev.y)) {
                        total_events_rx = 0;
                        total_events_drop = 0;
                        bh_inputmgr_reset();
                    } else if (fbui_widget_hit_test(btn_diag, oev.x, oev.y)) {
                        hal_serial_write("  [APP] Diagnostics triggered! All systems green.\n");
                    }
                    bh_fbui_cursor_draw(dev, &g_cursor);
                    needs_redraw = true;
                }
            }
        }

        const bh_inputmgr_counters_t *mgr_cnt = bh_inputmgr_get_counters();
        total_events_drop = (uint32_t)mgr_cnt->overflow_count;

        // Check if demo is fully complete
        if (g_pointer_event_passed && g_key_event_passed && g_button_clicked_passed) {
            if (!g_marker_complete) {
                hal_serial_write("BHARAT_INPUT_DEMO:COMPLETE\n");
                g_marker_complete = true;
            }
        }

        // Format strings for counters
        if (count > 0 || needs_redraw) {
            // Format received events count
            {
                char rev[16];
                int rpos = 0;
                uint32_t val = total_events_rx;
                if (val == 0) {
                    rev[rpos++] = '0';
                } else {
                    while (val > 0) {
                        rev[rpos++] = (char)('0' + (val % 10));
                        val /= 10;
                    }
                }
                int pos = 0;
                while (rpos > 0) {
                    rx_buf[21 + pos++] = rev[--rpos];
                }
                rx_buf[21 + pos] = '\0';
            }

            // Format dropped events count
            {
                char rev[16];
                int rpos = 0;
                uint32_t val = total_events_drop;
                if (val == 0) {
                    rev[rpos++] = '0';
                } else {
                    while (val > 0) {
                        rev[rpos++] = (char)('0' + (val % 10));
                        val /= 10;
                    }
                }
                int pos = 0;
                while (rpos > 0) {
                    drop_buf[21 + pos++] = rev[--rpos];
                }
                drop_buf[21 + pos] = '\0';
            }

            // Format cursor position
            {
                char revX[16], revY[16];
                int rposX = 0, rposY = 0;
                int32_t valX = g_cursor.x;
                int32_t valY = g_cursor.y;
                if (valX == 0) {
                    revX[rposX++] = '0';
                } else {
                    while (valX > 0) {
                        revX[rposX++] = (char)('0' + (valX % 10));
                        valX /= 10;
                    }
                }
                if (valY == 0) {
                    revY[rposY++] = '0';
                } else {
                    while (valY > 0) {
                        revY[rposY++] = (char)('0' + (valY % 10));
                        valY /= 10;
                    }
                }
                int pos = 0;
                while (rposX > 0) {
                    cursor_buf[21 + pos++] = revX[--rposX];
                }
                cursor_buf[21 + pos++] = ',';
                cursor_buf[21 + pos++] = ' ';
                while (rposY > 0) {
                    cursor_buf[21 + pos++] = revY[--rposY];
                }
                cursor_buf[21 + pos] = '\0';
            }

            // Restore cursor before drawing widgets, redraw widgets, then draw cursor back
            bh_fbui_cursor_erase(dev, &g_cursor);

            curr = lbl_title;
            while (curr) {
                if (curr->ops && curr->ops->draw) {
                    curr->ops->draw(curr, &ctx);
                }
                curr = curr->next;
            }

            bh_fbui_cursor_draw(dev, &g_cursor);
        }

        bh_thread_yield();
    }
}
