#include "match.h"
#include "driver_registry.h"
#include <stddef.h>

static int _strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int driver_match_device(device_desc_t* dev) {
    if (!dev || !dev->compatible_id) return -1;

    int drv_capacity;
    driver_desc_t** drivers = driver_registry_get_all(&drv_capacity);

    for (int i = 0; i < drv_capacity; i++) {
        driver_desc_t* drv = drivers[i];
        if (drv && drv->match_compatible_id) {
            if (_strcmp(dev->compatible_id, drv->match_compatible_id) == 0) {
                if (drv->probe) {
                    int ret = drv->probe(dev);
                    if (ret == 0) {
                        return 0; // successfully matched and probed
                    }
                }
            }
        }
    }
    return -1; // no match found or probe failed
}