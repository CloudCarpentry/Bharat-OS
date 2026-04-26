#ifndef BHARAT_DRIVER_MATCH_H
#define BHARAT_DRIVER_MATCH_H

#include "driver_core.h"

/**
 * @brief Match a device with the best available driver.
 *
 * This function follows a deterministic scoring model:
 * 1. Highest match score wins.
 * 2. If scores are equal, highest priority wins.
 * 3. If both are equal, returns -SYS_EBUSY (ambiguous).
 *
 * @return 0 on success, or a negative error code.
 */
int driver_match_device(device_desc_t* dev);

#endif // BHARAT_DRIVER_MATCH_H
