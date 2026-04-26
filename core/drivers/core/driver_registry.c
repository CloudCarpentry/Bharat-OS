#include "driver_registry.h"
#include "driver_core_internal.h"
#include "bharat/uapi/sys_errno.h"
#include <stddef.h>

#define MAX_DRIVERS 64
static driver_desc_t* g_drivers[MAX_DRIVERS];
static int g_driver_count = 0;
static uint32_t g_next_driver_registry_id = 1;

int driver_registry_init(void) {
    g_driver_count = 0;
    g_next_driver_registry_id = 1;
    for(int i = 0; i < MAX_DRIVERS; i++) {
        g_drivers[i] = NULL;
    }
    return 0;
}

int driver_register(driver_desc_t* drv) {
    /* Current contract: driver/device registration is boot-time or single-writer only.
     * Runtime hotplug requires adding locking/RCU/sequence-counter protection
     * before concurrent mutation is enabled. */

    if (!drv) return -SYS_EINVAL;
    if (!driver_core_name_valid(drv->name)) return -SYS_EINVAL;

    if (g_driver_count >= MAX_DRIVERS) return -SYS_ENOSPC;

    // Check for duplicates
    if (driver_find_by_name(drv->name)) return -SYS_EEXIST;

    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (g_drivers[i] == NULL) {
            drv->driver_registry_id = g_next_driver_registry_id++;
            g_drivers[i] = drv;
            g_driver_count++;
            return 0;
        }
    }
    return -SYS_ENOSPC;
}

void driver_unregister(driver_desc_t* drv) {
    if (!drv) return;
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (g_drivers[i] == drv) {
            g_drivers[i] = NULL;
            g_driver_count--;
            // Registry IDs are generally not reused for simplicity in D0
            return;
        }
    }
}

driver_desc_t** driver_registry_get_all(int* count_out) {
    if (count_out) *count_out = MAX_DRIVERS;
    return g_drivers;
}

int driver_registry_get_count(void) {
    return g_driver_count;
}

driver_desc_t* driver_find_by_name(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (g_drivers[i] && driver_core_streq(g_drivers[i]->name, name)) {
            return g_drivers[i];
        }
    }
    return NULL;
}
