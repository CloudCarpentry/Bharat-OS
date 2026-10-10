/* SPDX-License-Identifier: MIT */
#include "bharat_shell.h"
#include "bharat/ui/theme.h"
#include "bharat/uapi/boot/boot_events.h"

#include "lvgl.h"

#define BH_SHELL_HISTORY_DEPTH 8

static bh_shell_screen_id_t history[BH_SHELL_HISTORY_DEPTH];
static size_t history_count;
static bh_shell_screen_id_t current_screen = BH_SHELL_SCREEN_SPLASH;
static lv_obj_t *root;
static lv_obj_t *live_label;
static lv_timer_t *live_timer;
static lv_group_t *navigation_group;

static void apply_screen_style(lv_obj_t *screen) {
    const bh_ui_theme_t *theme = bh_theme_get_active();
    uint32_t bg = theme ? theme->bg_color_rgb : 0x081426;
    uint32_t fg = theme ? theme->text_color_rgb : 0xF5F7FA;

    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen, lv_color_hex(bg), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(fg), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

static void apply_button_style(lv_obj_t *button) {
    const bh_ui_theme_t *theme = bh_theme_get_active();
    uint32_t primary = theme ? theme->primary_color_rgb : 0xFF9933;

    /* Base style */
    lv_obj_set_style_bg_color(button, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);

    /* Focused style (high contrast keyboard/keypad highlight) */
    lv_obj_set_style_bg_color(button, lv_color_hex(0x334155), LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(button, lv_color_hex(primary), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(button, 3, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(button, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(button, lv_color_hex(primary), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(button, 2, LV_STATE_FOCUSED);

    /* Pressed style */
    lv_obj_set_style_bg_color(button, lv_color_hex(0x475569), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(button, lv_color_hex(0xF97316), LV_STATE_PRESSED);
}

static void add_header(lv_obj_t *parent, const char *title) {
    const bh_ui_theme_t *theme = bh_theme_get_active();
    const char *brand_name = (theme && theme->brand_name) ? theme->brand_name : "BHARAT-OS";
    uint32_t primary = theme ? theme->primary_color_rgb : 0xFF9933;

    lv_obj_t *brand = lv_label_create(parent);
    lv_label_set_text(brand, brand_name);
    lv_obj_set_style_text_color(brand, lv_color_hex(primary), 0);
    lv_obj_align(brand, LV_ALIGN_TOP_LEFT, 28, 22);

    lv_obj_t *heading = lv_label_create(parent);
    lv_label_set_text(heading, title);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 22);
}

static void back_event(lv_event_t *event) {
    (void)event;
    (void)bh_shell_navigate_back();
}

static void key_event_handler(lv_event_t *event) {
    uint32_t key = lv_event_get_key(event);
    if (key == LV_KEY_ESC) {
        (void)bh_shell_navigate_back();
    }
}

static void add_back_button(lv_obj_t *parent) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 100, 42);
    lv_obj_align(button, LV_ALIGN_BOTTOM_LEFT, 28, -22);
    apply_button_style(button);
    lv_obj_add_event_cb(button, back_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(button, key_event_handler, LV_EVENT_KEY, NULL);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "< Back");
    lv_obj_center(label);
    if (navigation_group != NULL) {
        lv_group_add_obj(navigation_group, button);
        lv_group_focus_obj(button);
    }
}

static void navigation_event(lv_event_t *event) {
    bh_shell_screen_id_t target = (bh_shell_screen_id_t)(uintptr_t)lv_event_get_user_data(event);
    (void)bh_shell_navigate(target);
}

static void add_launcher_button(lv_obj_t *parent, const char *label, bh_shell_screen_id_t target) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 210, 72);
    apply_button_style(button);
    lv_obj_add_event_cb(button, navigation_event, LV_EVENT_CLICKED, (void *)(uintptr_t)target);
    lv_obj_add_event_cb(button, key_event_handler, LV_EVENT_KEY, NULL);
    lv_obj_t *lbl = lv_label_create(button);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    if (navigation_group != NULL) {
        lv_group_add_obj(navigation_group, button);
    }
}

static void update_live_label(lv_timer_t *timer) {
    bh_shell_system_info_t info;
    char text[128];
    (void)timer;
    if (live_label == NULL) return;
    bh_shell_snapshot(&info);
    (void)lv_snprintf(text, sizeof(text), "Uptime  %02llu:%02llu:%02llu     Heap  %u KB used / %u KB free",
                   (unsigned long long)(info.uptime_seconds / 3600U),
                   (unsigned long long)((info.uptime_seconds / 60U) % 60U),
                   (unsigned long long)(info.uptime_seconds % 60U), info.heap_used_kb,
                   info.heap_free_kb);
    lv_label_set_text(live_label, text);
}

static void create_launcher(void) {
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    lv_obj_add_event_cb(root, key_event_handler, LV_EVENT_KEY, NULL);
    add_header(root, "Desktop / Launcher");
    lv_obj_t *grid = lv_obj_create(root);
    lv_obj_set_size(grid, 720, 470);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    add_launcher_button(grid, "System", BH_SHELL_SCREEN_SYSTEM);
    add_launcher_button(grid, "Devices", BH_SHELL_SCREEN_DEVICES);
    add_launcher_button(grid, "Boot Diagnostics", BH_SHELL_SCREEN_DIAGNOSTICS);
    add_launcher_button(grid, "Processes", BH_SHELL_SCREEN_DEMOS);
    add_launcher_button(grid, "Network", BH_SHELL_SCREEN_DEMOS);
    add_launcher_button(grid, "Demo Apps", BH_SHELL_SCREEN_DEMOS);
    if (navigation_group != NULL) {
        lv_group_focus_next(navigation_group);
    }
}

static void create_system(void) {
    static const struct { uint32_t mask; const char *name; } caps[] = {
        {BH_SHELL_CAP_MMU, "MMU"}, {BH_SHELL_CAP_SMP, "SMP"},
        {BH_SHELL_CAP_TIMER, "Timer"}, {BH_SHELL_CAP_ATOMIC, "Atomic"},
        {BH_SHELL_CAP_DISPLAY, "Display"}, {BH_SHELL_CAP_NETWORK, "Network"},
    };
    bh_shell_system_info_t info;
    char text[512];
    size_t used;
    bh_shell_snapshot(&info);
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    lv_obj_add_event_cb(root, key_event_handler, LV_EVENT_KEY, NULL);
    add_header(root, "System");

    const char *arch_str = (info.architecture && info.architecture[0]) ? info.architecture : "Unavailable";
    const char *profile_str = (info.profile && info.profile[0]) ? info.profile : "N/A";
    const char *runtime_str = (info.runtime && info.runtime[0]) ? info.runtime : "N/A";
    const char *kernel_str = (info.kernel_build && info.kernel_build[0]) ? info.kernel_build : "N/A";

    char cpu_str[32];
    if (info.cpu_cores > 0) {
        (void)lv_snprintf(cpu_str, sizeof(cpu_str), "%u", info.cpu_cores);
    } else {
        (void)lv_snprintf(cpu_str, sizeof(cpu_str), "N/A");
    }

    char mem_str[32];
    if (info.memory_total_mb > 0) {
        (void)lv_snprintf(mem_str, sizeof(mem_str), "%u MB", info.memory_total_mb);
    } else {
        (void)lv_snprintf(mem_str, sizeof(mem_str), "N/A");
    }

    (void)lv_snprintf(text, sizeof(text),
                   "Bharat-OS\n\nArchitecture : %s\nCPU cores    : %s\nMemory       : %s\n"
                   "Profile      : %s\nRuntime      : %s\nKernel       : %s\n\nHardware capabilities\n",
                   arch_str, cpu_str, mem_str, profile_str, runtime_str, kernel_str);
    used = 0;
    while (text[used] != '\0') used++;
    for (size_t i = 0; i < sizeof(caps) / sizeof(caps[0]); ++i) {
        used += (size_t)lv_snprintf(text + used, sizeof(text) - used, "%s %s\n",
                                (info.capability_mask & caps[i].mask) ? "[+] " : "[-] ", caps[i].name);
    }
    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 92);
    live_label = lv_label_create(root);
    lv_obj_align(live_label, LV_ALIGN_BOTTOM_RIGHT, -28, -30);
    update_live_label(NULL);
    live_timer = lv_timer_create(update_live_label, 1000, NULL);
    add_back_button(root);
}

static void create_devices(void) {
    char text[512] = "Discovered Devices & Drivers\n\n";
    size_t count = 0;
    size_t used = 0;
    while (text[used] != '\0') used++;
    const bh_shell_device_t *devices = bh_shell_devices(&count);
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    lv_obj_add_event_cb(root, key_event_handler, LV_EVENT_KEY, NULL);
    add_header(root, "Device Viewer");
    if (devices == NULL || count == 0) {
        used += (size_t)lv_snprintf(text + used, sizeof(text) - used,
                                   "No hardware devices enumerated.\nDevice discovery service offline or uninitialized.\n");
    } else {
        for (size_t i = 0; i < count && used + 64 < sizeof(text); ++i) {
            used += (size_t)lv_snprintf(text + used, sizeof(text) - used, "%-14s  %-18s  %s\n",
                                    devices[i].category ? devices[i].category : "Unknown",
                                    devices[i].driver ? devices[i].driver : "Generic",
                                    devices[i].status ? devices[i].status : "UNKNOWN");
        }
    }
    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 105);
    add_back_button(root);
}

static void create_diagnostics(void) {
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    lv_obj_add_event_cb(root, key_event_handler, LV_EVENT_KEY, NULL);
    add_header(root, "Kernel & Service Diagnostics");

    bh_boot_event_snapshot_t snap;
    bh_boot_events_get_snapshot(&snap);

    char log_text[1024];
    size_t used = 0;

    used += (size_t)lv_snprintf(log_text + used, sizeof(log_text) - used,
                               "Total Boot Events: %u | Dropped: %u\n\n",
                               snap.total_events, snap.dropped_events);

    if (snap.count == 0) {
        used += (size_t)lv_snprintf(log_text + used, sizeof(log_text) - used,
                                   "No boot events recorded yet.\n");
    } else {
        for (uint32_t i = 0; i < snap.count && used + 64 < sizeof(log_text); ++i) {
            const bh_boot_event_record_t *ev = &snap.events[i];
            used += (size_t)lv_snprintf(log_text + used, sizeof(log_text) - used,
                                       "[%-9s] %-10s : %-12s (%s)\n",
                                       bh_boot_stage_name(ev->stage),
                                       ev->component,
                                       ev->message,
                                       bh_boot_status_name(ev->status));
        }
    }

    lv_obj_t *log_container = lv_obj_create(root);
    lv_obj_set_size(log_container, 720, 360);
    lv_obj_align(log_container, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_color(log_container, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_border_color(log_container, lv_color_hex(0x334155), 0);

    lv_obj_t *label = lv_label_create(log_container);
    lv_label_set_text(label, log_text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);

    add_back_button(root);
}

static void create_demos(void) {
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    lv_obj_add_event_cb(root, key_event_handler, LV_EVENT_KEY, NULL);
    add_header(root, "Demo Apps");
    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, "Process, network, hardware, and sensor demos\nare ready for service-backed views.");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
    add_back_button(root);
}

static void create_splash(void) {
    const bh_ui_theme_t *theme = bh_theme_get_active();
    const char *brand_name = (theme && theme->brand_name) ? theme->brand_name : "BHARAT-OS";
    const char *tagline = (theme && theme->tagline) ? theme->tagline : "Booting kernel modules...";
    uint32_t primary = theme ? theme->primary_color_rgb : 0xFF9933;
    bool show_spinner = theme ? theme->show_spinner : true;

    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);

    /* Animated spinner/arc */
    if (show_spinner) {
        lv_obj_t *spinner = lv_spinner_create(root);
        lv_obj_set_size(spinner, 100, 100);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -40);
        lv_obj_set_style_arc_color(spinner, lv_color_hex(primary), LV_PART_INDICATOR);
    }

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, brand_name);
    lv_obj_set_style_text_color(title, lv_color_hex(primary), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle, tagline);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 65);

    /* Real kernel progress log from boot event snapshot */
    bh_boot_event_snapshot_t snap;
    bh_boot_events_get_snapshot(&snap);

    char log_buf[256];
    if (snap.count > 0) {
        const bh_boot_event_record_t *last_ev = &snap.events[snap.count - 1];
        (void)lv_snprintf(log_buf, sizeof(log_buf), "[%s] %s: %s",
                          bh_boot_stage_name(last_ev->stage),
                          last_ev->component,
                          last_ev->message);
    } else {
        (void)lv_snprintf(log_buf, sizeof(log_buf), "[BOOT] Starting system services...");
    }

    lv_obj_t *log_label = lv_label_create(root);
    lv_label_set_text(log_label, log_buf);
    lv_obj_set_style_text_color(log_label, lv_color_hex(0xA0A0A0), 0);
    lv_obj_align(log_label, LV_ALIGN_BOTTOM_LEFT, 20, -20);
}

static void load_screen(bh_shell_screen_id_t target) {
    if (live_timer != NULL) {
        lv_timer_delete(live_timer);
        live_timer = NULL;
    }
    live_label = NULL;
    if (root != NULL) {
        lv_obj_delete(root);
        root = NULL;
    }
    if (navigation_group != NULL) {
        lv_group_remove_all_objs(navigation_group);
    }
    current_screen = target;
    switch (target) {
        case BH_SHELL_SCREEN_SPLASH: create_splash(); break;
        case BH_SHELL_SCREEN_LAUNCHER: create_launcher(); break;
        case BH_SHELL_SCREEN_SYSTEM: create_system(); break;
        case BH_SHELL_SCREEN_DEVICES: create_devices(); break;
        case BH_SHELL_SCREEN_DIAGNOSTICS: create_diagnostics(); break;
        case BH_SHELL_SCREEN_DEMOS: create_demos(); break;
        default: create_launcher(); current_screen = BH_SHELL_SCREEN_LAUNCHER; break;
    }
}

int bh_shell_navigate(bh_shell_screen_id_t target) {
    if (target >= BH_SHELL_SCREEN_COUNT) return -1;
    if (target == current_screen) return 0;

    /* Transitioning from SPLASH to LAUNCHER initializes LAUNCHER as the root screen */
    if (current_screen == BH_SHELL_SCREEN_SPLASH && target == BH_SHELL_SCREEN_LAUNCHER) {
        history_count = 0;
    } else {
        if (history_count >= BH_SHELL_HISTORY_DEPTH) return -1;
        history[history_count++] = current_screen;
    }

    load_screen(target);
    return 0;
}

int bh_shell_navigate_back(void) {
    if (history_count == 0) return -1;
    load_screen(history[--history_count]);
    return 0;
}

bh_shell_screen_id_t bh_shell_current_screen(void) { return current_screen; }

void bh_shell_start(void) {
    history_count = 0;
    if (navigation_group == NULL) {
        navigation_group = lv_group_create();
    } else {
        lv_group_remove_all_objs(navigation_group);
    }
    load_screen(BH_SHELL_SCREEN_SPLASH);
}

lv_group_t *bh_shell_navigation_group(void) { return navigation_group; }

