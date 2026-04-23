#include "driver_registry.h"
#include <stddef.h>

#define MAX_DRIVERS 64
static driver_desc_t* g_drivers[MAX_DRIVERS];
static int g_driver_count = 0;

int driver_registry_init(void) {
    g_driver_count = 0;
    for(int i = 0; i < MAX_DRIVERS; i++) {
        g_drivers[i] = NULL;
    }
    return 0;
}

int driver_register(driver_desc_t* drv) {
    if (!drv || g_driver_count >= MAX_DRIVERS) return -1;

    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (g_drivers[i] == NULL) {
            g_drivers[i] = drv;
            g_driver_count++;
            return 0;
        }
    }
    return -1;
}

void driver_unregister(driver_desc_t* drv) {
    if (!drv) return;
    for (int i = 0; i < MAX_DRIVERS; i++) {
        if (g_drivers[i] == drv) {
            g_drivers[i] = NULL;
            g_driver_count--;
            return;
        }
    }
}

driver_desc_t** driver_registry_get_all(int* count_out) {
    if (count_out) *count_out = MAX_DRIVERS;
    return g_drivers;
}