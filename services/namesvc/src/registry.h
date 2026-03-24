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
 * @brief Add a version-aware interface mapping to the registry.
 */
int32_t namesvc_registry_add(const char *service_name,
                             const char *interface_name,
                             uint32_t interface_version,
                             uint32_t transport_flags,
                             bharat_cap_handle_t endpoint);

/**
 * @brief Lookup an interface mapping in the registry.
 */
int32_t namesvc_registry_lookup(const char *service_name,
                                const char *interface_name,
                                uint32_t requested_version,
                                bool exact_version,
                                bharat_cap_handle_t *endpoint,
                                uint32_t *out_version,
                                uint32_t *out_transport_flags);

/**
 * @brief Remove an interface mapping from the registry.
 */
int32_t namesvc_registry_remove(const char *service_name,
                                const char *interface_name,
                                uint32_t interface_version);

#endif // NAMESVC_REGISTRY_H
