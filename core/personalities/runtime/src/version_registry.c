#include "bharat/subsys/version.h"
#include "bh_personality_version.h"

#ifndef BHARAT_KERNEL_VERSION
#define BHARAT_KERNEL_VERSION "unknown"
#endif

static const bharat_subsys_version_entry_t g_subsys_versions[] = {
    {"subsys-manager", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"linux-personality", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"windows-personality", BHARAT_KERNEL_VERSION, 1U, 1U, "dev"},
    {"android-personality", BHARAT_KERNEL_VERSION, 1U, 1U, "experimental"},
    {"automotive-subsys", BHARAT_KERNEL_VERSION, 1U, 1U, "beta"},
    {"network-subsys", BHARAT_KERNEL_VERSION, 2U, 2U, "beta"},
};

const char *bharat_subsys_kernel_version(void) {
    return BHARAT_KERNEL_VERSION;
}

size_t bharat_subsys_version_count(void) {
    return sizeof(g_subsys_versions) / sizeof(g_subsys_versions[0]);
}

const bharat_subsys_version_entry_t *bharat_subsys_version_at(size_t index) {
    if (index >= bharat_subsys_version_count()) {
        return NULL;
    }

    return &g_subsys_versions[index];
}
