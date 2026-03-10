#ifndef BHARAT_ANDROID_COMPAT_H
#define BHARAT_ANDROID_COMPAT_H

#include "subsys.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initialize the Android personality subsystem instance.
 * @param instance The subsystem instance being initialized.
 * @return 0 on success, < 0 on error.
 */
int android_subsys_init(subsys_instance_t* instance);

/**
 * @brief Android personality startup hook.
 *
 * This hook sets up the core Android userspace primitives (Service Manager,
 * ashmem layer, Binder interfaces) relying on the core kernel Linux-like
 * primitives (VFS, IPC, memory object model).
 */
int android_subsys_start(subsys_instance_t* instance);

/**
 * @brief Placeholder for Binder IPC initialization.
 *
 * Maps generic kernel IPC endpoints and event objects to Android's
 * Binder node and handle concepts without hardwiring Binder semantics
 * directly into the core kernel.
 */
int binder_compat_init(void);

/**
 * @brief Placeholder for Ashmem compatibility initialization.
 *
 * Wraps core kernel shared memory objects (e.g., named/anonymous memory
 * with sealing capabilities) into an ashmem-like abstraction layer.
 */
int ashmem_compat_init(void);

/**
 * @brief Placeholder for Android Service Manager integration.
 *
 * Integrates Android's HAL service discovery with Bharat-OS's
 * capability-checked service registry.
 */
int android_service_manager_init(void);

#endif // BHARAT_ANDROID_COMPAT_H
