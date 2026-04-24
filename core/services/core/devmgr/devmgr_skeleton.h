#ifndef BHARAT_DEVMGR_SKELETON_H
#define BHARAT_DEVMGR_SKELETON_H

#include "../../../drivers/include/driver_core.h"

// Initialize the device manager policy service (skeleton)
int devmgr_init(void);

// Provide a clean way to inspect the number of devices tracked by the devmgr
int devmgr_get_tracked_device_count(void);

#endif // BHARAT_DEVMGR_SKELETON_H