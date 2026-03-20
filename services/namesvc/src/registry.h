#ifndef NAMESVC_REGISTRY_H
#define NAMESVC_REGISTRY_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/cap/cap.h>
#include <bharat/namesvc/namesvc_ipc.h>

/**
 * @file registry.h
 * @brief Endpoint registry for the Name Service.
 */

/**
 * @brief Initialize the name registry.
 */
void namesvc_registry_init(void);

/**
 * @brief Add a name mapping to the registry.
 * @param name Null-terminated name string.
 * @param endpoint The capability handle for the endpoint.
 * @return NAMESVC_STATUS_OK on success, error code otherwise.
 */
int32_t namesvc_registry_add(const char *name, bharat_cap_handle_t endpoint);

/**
 * @brief Lookup a name mapping in the registry.
 * @param name Null-terminated name string.
 * @param endpoint Output pointer for the capability handle.
 * @return NAMESVC_STATUS_OK on success, error code otherwise.
 */
int32_t namesvc_registry_lookup(const char *name, bharat_cap_handle_t *endpoint);

/**
 * @brief Remove a name mapping from the registry.
 * @param name Null-terminated name string.
 * @return NAMESVC_STATUS_OK on success, error code otherwise.
 */
int32_t namesvc_registry_remove(const char *name);

#endif // NAMESVC_REGISTRY_H
