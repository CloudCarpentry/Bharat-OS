/* SPDX-License-Identifier: MIT */
#include "bharat_shell.h"

#include <string.h>

static bh_shell_snapshot_fn snapshot_provider;
static void *snapshot_context;

static const bh_shell_device_t devices[] = {
    {"Display", "virtio-gpu", "READY"},
    {"Keyboard", "virtio-input", "READY"},
    {"Network", "virtio-net", "READY"},
    {"Storage", "virtio-blk", "READY"},
    {"Sensor", "virtual-imu", "READY"},
    {"Accelerator", "virt-npu", "READY"},
};

void bh_shell_set_snapshot_provider(bh_shell_snapshot_fn provider, void *context) {
    snapshot_provider = provider;
    snapshot_context = context;
}

void bh_shell_snapshot(bh_shell_system_info_t *info) {
    if (info == NULL) {
        return;
    }

    memset(info, 0, sizeof(*info));
    info->architecture = "x86_64";
    info->cpu_cores = 4;
    info->memory_total_mb = 512;
    info->profile = "DESKTOP";
    info->runtime = "LIGHT";
    info->kernel_build = "Debug";
    info->heap_used_kb = 896;
    info->heap_free_kb = 7296;
    info->capability_mask = BH_SHELL_CAP_MMU | BH_SHELL_CAP_SMP | BH_SHELL_CAP_TIMER |
                            BH_SHELL_CAP_ATOMIC | BH_SHELL_CAP_DISPLAY | BH_SHELL_CAP_NETWORK;

    /* The service adapter replaces demo defaults with capability-mediated data. */
    if (snapshot_provider != NULL) {
        snapshot_provider(info, snapshot_context);
    }
}

const bh_shell_device_t *bh_shell_devices(size_t *count) {
    if (count != NULL) {
        *count = sizeof(devices) / sizeof(devices[0]);
    }
    return devices;
}
