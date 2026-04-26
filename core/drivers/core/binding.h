#ifndef BHARAT_DRIVER_BINDING_H
#define BHARAT_DRIVER_BINDING_H

#include "driver_core.h"
#include "bharat/uapi/sys_errno.h"

/**
 * @brief Initialize the binding registry.
 */
int device_binding_registry_init(void);

/**
 * @brief Create a new device binding.
 */
device_binding_t* device_binding_create(device_desc_t* dev, driver_desc_t* drv, int score);

/**
 * @brief Find binding for a device.
 */
device_binding_t* device_binding_find_by_dev(device_desc_t* dev);

/**
 * @brief Transition a binding to the PROBED state.
 */
int device_binding_probe(device_binding_t* binding);

/**
 * @brief Transition a binding to the STARTED state.
 */
int device_binding_start(device_binding_t* binding);

/**
 * @brief Transition a binding to the STOPPED state.
 */
int device_binding_stop(device_binding_t* binding);

/**
 * @brief Transition a binding to the REMOVED state.
 */
int device_binding_remove(device_binding_t* binding);

/**
 * @brief Force a binding into the FAILED state.
 */
void device_binding_fail(device_binding_t* binding);

#endif // BHARAT_DRIVER_BINDING_H
