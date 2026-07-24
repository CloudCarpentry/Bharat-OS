#ifndef BHARAT_NAMESVC_CLIENT_H
#define BHARAT_NAMESVC_CLIENT_H

#include <bharat/uapi/namesvc/contract.h>
#include <bharat/ipc/ipc.h>

/**
 * @brief Registers a service with namesvc.
 */
int namesvc_register(const char *name,
                     bharat_service_id_t service_id,
                     bharat_ipc_endpoint_t endpoint,
                     uint32_t version,
                     uint32_t flags);

/**
 * @brief Looks up a service by name.
 */
int namesvc_lookup(const char *name,
                   bharat_service_id_t *out_service_id,
                   bharat_ipc_endpoint_t *out_endpoint,
                   uint32_t *out_version);

#endif // BHARAT_NAMESVC_CLIENT_H
