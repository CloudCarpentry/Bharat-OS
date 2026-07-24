#ifndef BHARAT_UAPI_SERVICES_SERVICE_IDS_H
#define BHARAT_UAPI_SERVICES_SERVICE_IDS_H

#include <stdint.h>

/**
 * @file service_ids.h
 * @brief Canonical service identifiers for Bharat-OS.
 */

typedef uint32_t bharat_service_id_t;

#define BHARAT_SERVICE_NAMESVC          1u
#define BHARAT_SERVICE_SERVICEMGR       2u
#define BHARAT_SERVICE_PROCESS_MANAGER  3u
#define BHARAT_SERVICE_VM_MANAGER       4u
#define BHARAT_SERVICE_NETMGR           10u
#define BHARAT_SERVICE_NETSTACK         11u

#endif // BHARAT_UAPI_SERVICES_SERVICE_IDS_H
