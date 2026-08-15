/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_SHELL_H
#define BHARAT_SHELL_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BH_SHELL_SCREEN_SPLASH = 0,
    BH_SHELL_SCREEN_LAUNCHER,
    BH_SHELL_SCREEN_SYSTEM,
    BH_SHELL_SCREEN_DEVICES,
    BH_SHELL_SCREEN_DEMOS,
    BH_SHELL_SCREEN_COUNT
} bh_shell_screen_id_t;

typedef struct {
    const char *architecture;
    uint32_t cpu_cores;
    uint32_t memory_total_mb;
    const char *profile;
    const char *runtime;
    const char *kernel_build;
    uint64_t uptime_seconds;
    uint32_t heap_used_kb;
    uint32_t heap_free_kb;
    uint32_t capability_mask;
} bh_shell_system_info_t;

enum {
    BH_SHELL_CAP_MMU = 1U << 0,
    BH_SHELL_CAP_SMP = 1U << 1,
    BH_SHELL_CAP_TIMER = 1U << 2,
    BH_SHELL_CAP_ATOMIC = 1U << 3,
    BH_SHELL_CAP_DISPLAY = 1U << 4,
    BH_SHELL_CAP_NETWORK = 1U << 5,
};

typedef struct {
    const char *category;
    const char *driver;
    const char *status;
} bh_shell_device_t;

typedef void (*bh_shell_snapshot_fn)(bh_shell_system_info_t *info, void *context);

void bh_shell_set_snapshot_provider(bh_shell_snapshot_fn provider, void *context);
void bh_shell_snapshot(bh_shell_system_info_t *info);
const bh_shell_device_t *bh_shell_devices(size_t *count);
int bh_shell_navigate(bh_shell_screen_id_t target);
int bh_shell_navigate_back(void);
bh_shell_screen_id_t bh_shell_current_screen(void);
void bh_shell_start(void);

struct _lv_group_t;
struct _lv_group_t *bh_shell_navigation_group(void);

#endif
