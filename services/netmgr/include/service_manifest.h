#ifndef NETMGR_SERVICE_MANIFEST_H
#define NETMGR_SERVICE_MANIFEST_H

#include <uapi/bharat/ipc/manifest.h>

#ifdef __cplusplus
extern "C" {
#endif

// Expose the static manifest table for netmgr
extern const bharat_ipc_service_manifest_t netmgr_service_manifest;

#ifdef __cplusplus
}
#endif

#endif // NETMGR_SERVICE_MANIFEST_H
