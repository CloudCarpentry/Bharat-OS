/* SPDX-License-Identifier: MIT */
#include "bharat_shell.h"

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
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x081426), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xf5f7fa), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

static void add_header(lv_obj_t *parent, const char *title) {
    lv_obj_t *brand = lv_label_create(parent);
    lv_label_set_text(brand, "BHARAT-OS");
    lv_obj_set_style_text_color(brand, lv_color_hex(0xff9933), 0);
    lv_obj_align(brand, LV_ALIGN_TOP_LEFT, 28, 22);

    lv_obj_t *heading = lv_label_create(parent);
    lv_label_set_text(heading, title);
    lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 22);
}

static void back_event(lv_event_t *event) {
    (void)event;
    (void)bh_shell_navigate_back();
}

static void add_back_button(lv_obj_t *parent) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_align(button, LV_ALIGN_BOTTOM_LEFT, 28, -22);
    lv_obj_add_event_cb(button, back_event, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(button), "Back");
    lv_group_add_obj(navigation_group, button);
}

static void navigation_event(lv_event_t *event) {
    bh_shell_screen_id_t target = (bh_shell_screen_id_t)(uintptr_t)lv_event_get_user_data(event);
    (void)bh_shell_navigate(target);
}

static void add_launcher_button(lv_obj_t *parent, const char *label, bh_shell_screen_id_t target) {
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 210, 72);
    lv_obj_add_event_cb(button, navigation_event, LV_EVENT_CLICKED, (void *)(uintptr_t)target);
    lv_label_set_text(lv_label_create(button), label);
    lv_group_add_obj(navigation_group, button);
}

static void update_live_label(lv_timer_t *timer) {
    bh_shell_system_info_t info;
    char text[128];
    (void)timer;
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
    add_header(root, "Desktop / Launcher");
    lv_obj_t *grid = lv_obj_create(root);
    lv_obj_set_size(grid, 720, 470);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER);
    add_launcher_button(grid, "System", BH_SHELL_SCREEN_SYSTEM);
    add_launcher_button(grid, "Devices", BH_SHELL_SCREEN_DEVICES);
    add_launcher_button(grid, "Processes", BH_SHELL_SCREEN_DEMOS);
    add_launcher_button(grid, "Network", BH_SHELL_SCREEN_DEMOS);
    add_launcher_button(grid, "Hardware & Sensors", BH_SHELL_SCREEN_DEMOS);
    add_launcher_button(grid, "Demo Apps", BH_SHELL_SCREEN_DEMOS);
    lv_group_focus_next(navigation_group);
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
    add_header(root, "System");
    (void)lv_snprintf(text, sizeof(text),
                   "Bharat-OS\n\nArchitecture : %s\nCPU cores    : %u\nMemory       : %u MB\n"
                   "Profile      : %s\nRuntime      : %s\nKernel       : %s\n\nHardware capabilities\n",
                   info.architecture, info.cpu_cores, info.memory_total_mb, info.profile,
                   info.runtime, info.kernel_build);
    used = 0;
    while (text[used] != '\0') used++;
    for (size_t i = 0; i < sizeof(caps) / sizeof(caps[0]); ++i) {
        used += (size_t)lv_snprintf(text + used, sizeof(text) - used, "%s %s\n",
                                (info.capability_mask & caps[i].mask) ? "+" : "-", caps[i].name);
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
    char text[512] = "Devices\n\n";
    size_t count;
    size_t used = 10;
    const bh_shell_device_t *devices = bh_shell_devices(&count);
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    add_header(root, "Device Viewer");
    for (size_t i = 0; i < count; ++i) {
        used += (size_t)lv_snprintf(text + used, sizeof(text) - used, "%-12s  %-16s  %s\n",
                                devices[i].category, devices[i].driver, devices[i].status);
    }
    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 150, 105);
    add_back_button(root);
}

static void create_demos(void) {
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);
    add_header(root, "Demo Apps");
    lv_obj_t *label = lv_label_create(root);
    lv_label_set_text(label, "Process, network, hardware, and sensor demos\nare ready for service-backed views.");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
    add_back_button(root);
}

static void create_splash(void) {
    root = lv_obj_create(lv_screen_active());
    apply_screen_style(root);

    /* Animated spinner/arc */
    lv_obj_t *spinner = lv_spinner_create(root);
    lv_obj_set_size(spinner, 100, 100);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xff9933), LV_PART_INDICATOR);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "BHARAT-OS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xff9933), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle, "Booting kernel modules...");
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 65);

    /* Kernel progress log area */
    lv_obj_t *log_label = lv_label_create(root);
    lv_label_set_text(log_label, "[LOG] Starting display service...\n[LOG] Initializing IPC...\n[LOG] Calibrating hardware...");
    lv_obj_set_style_text_color(log_label, lv_color_hex(0xa0a0a0), 0);
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
    lv_group_remove_all_objs(navigation_group);
    current_screen = target;
    switch (target) {
        case BH_SHELL_SCREEN_SPLASH: create_splash(); break;
        case BH_SHELL_SCREEN_LAUNCHER: create_launcher(); break;
        case BH_SHELL_SCREEN_SYSTEM: create_system(); break;
        case BH_SHELL_SCREEN_DEVICES: create_devices(); break;
        case BH_SHELL_SCREEN_DEMOS: create_demos(); break;
        default: create_launcher(); current_screen = BH_SHELL_SCREEN_LAUNCHER; break;
    }
}

int bh_shell_navigate(bh_shell_screen_id_t target) {
    if (target >= BH_SHELL_SCREEN_COUNT || history_count >= BH_SHELL_HISTORY_DEPTH) return -1;
    history[history_count++] = current_screen;
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
    navigation_group = lv_group_create();
    load_screen(BH_SHELL_SCREEN_SPLASH);
}

lv_group_t *bh_shell_navigation_group(void) { return navigation_group; }
