#include "match.h"
#include "driver_registry.h"
#include "driver_core_internal.h"
#include "bharat/uapi/sys_errno.h"
#include <stddef.h>

#define MATCH_SCORE_COMPATIBLE   1000
#define MATCH_SCORE_VENDOR_DEV   500
#define MATCH_SCORE_CLASS        100
#define MATCH_SCORE_GENERIC      10

static int calculate_match_score(device_desc_t* dev, driver_desc_t* drv) {
    int score = 0;

    // 1. Exact compatible string match (highest priority)
    if (drv->match_compatible_id && dev->compatible_id) {
        if (driver_core_streq(dev->compatible_id, drv->match_compatible_id)) {
            score += MATCH_SCORE_COMPATIBLE;
        }
    }

    // 2. Vendor/Device ID match
    if (drv->match_vendor_id != 0 && drv->match_vendor_id != BH_DRIVER_MATCH_ANY_U32) {
        if (dev->vendor_id == drv->match_vendor_id) {
            if (drv->match_device_id != 0 && drv->match_device_id != BH_DRIVER_MATCH_ANY_U32) {
                if (dev->device_id == drv->match_device_id) {
                    score += MATCH_SCORE_VENDOR_DEV;
                }
            } else {
                // Vendor match only
                score += MATCH_SCORE_VENDOR_DEV / 2;
            }
        }
    }

    // 3. Class match
    if (drv->match_class != CLASS_UNKNOWN) {
        if (dev->dev_class == drv->match_class) {
            score += MATCH_SCORE_CLASS;
        }
    }

    // 4. Generic fallback (if it supports the class but doesn't have specific match criteria)
    if (score == 0 && drv->supported_class != CLASS_UNKNOWN) {
        if (dev->dev_class == drv->supported_class) {
            score += MATCH_SCORE_GENERIC;
        }
    }

    return score;
}

int driver_match_device(device_desc_t* dev) {
    if (!dev) return -SYS_EINVAL;

    int drv_capacity;
    driver_desc_t** drivers = driver_registry_get_all(&drv_capacity);

    driver_desc_t* best_drv = NULL;
    int best_score = 0;
    int best_index = -1;

    for (int i = 0; i < drv_capacity; i++) {
        driver_desc_t* drv = drivers[i];
        if (!drv) continue;

        int score = calculate_match_score(dev, drv);
        if (score > 0) {
            bool is_better = false;
            if (score > best_score) {
                is_better = true;
            } else if (score == best_score) {
                // Tie-breaker:
                // 1. higher driver priority
                if (drv->priority > best_drv->priority) {
                    is_better = true;
                } else if (drv->priority == best_drv->priority) {
                    // 2. earlier registration order (lower index)
                    if (best_index == -1 || i < best_index) {
                        is_better = true;
                    }
                    // 3. driver name lexical order (omitted for simplicity, registration order is stable)
                }
            }

            if (is_better) {
                best_score = score;
                best_drv = drv;
                best_index = i;
            }
        }
    }

    if (best_drv && best_drv->probe) {
        int ret = best_drv->probe(dev);
        if (ret == 0) {
            dev->driver_data = (void*)best_drv; // Mark as bound
            return 0;
        }
    }

    return -SYS_ENOENT;
}