/* SPDX-License-Identifier: MIT */
#include "bharat_shell.h"

#include <assert.h>
#include <string.h>

static void test_provider(bh_shell_system_info_t *info, void *context) {
    info->uptime_seconds = *(const uint64_t *)context;
    info->heap_used_kb = 1024;
}

int main(void) {
    bh_shell_system_info_t info;
    size_t count = 0;
    const uint64_t uptime = 3661;
    const bh_shell_device_t *devices;

    bh_shell_snapshot(&info);
    assert(strcmp(info.architecture, "x86_64") == 0);
    assert(info.cpu_cores == 4);
    assert((info.capability_mask & BH_SHELL_CAP_DISPLAY) != 0);

    bh_shell_set_snapshot_provider(test_provider, (void *)&uptime);
    bh_shell_snapshot(&info);
    assert(info.uptime_seconds == uptime);
    assert(info.heap_used_kb == 1024);

    devices = bh_shell_devices(&count);
    assert(count == 6);
    assert(strcmp(devices[0].driver, "virtio-gpu") == 0);
    assert(strcmp(devices[5].status, "READY") == 0);
    return 0;
}
