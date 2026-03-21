#include "bharat/drivers/version.h"

#include <bharat/version.h>

#ifndef BHARAT_KERNEL_VERSION
#define BHARAT_KERNEL_VERSION "unknown"
#endif

static const bharat_driver_version_entry_t g_driver_versions[] = {
    {"virtio-net", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"ptp-clock", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"fpga-mgr", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"cluster-bus", BHARAT_KERNEL_VERSION, 1U, 1U, "experimental"},
    {"automotive-can-loopback", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"generic-arch-irq-timer", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"generic-board-fabric", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
};

const char *bharat_driver_kernel_version(void) {
    return BHARAT_KERNEL_VERSION;
}

size_t bharat_driver_version_count(void) {
    return sizeof(g_driver_versions) / sizeof(g_driver_versions[0]);
}

const bharat_driver_version_entry_t *bharat_driver_version_at(size_t index) {
    if (index >= bharat_driver_version_count()) {
        return NULL;
    }

    return &g_driver_versions[index];
}
