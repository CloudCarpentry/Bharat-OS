#ifndef NAMESVC_SERVICE_MANIFEST_H
#define NAMESVC_SERVICE_MANIFEST_H

#include <uapi/bharat/ipc/manifest.h>

#ifdef __cplusplus
extern "C" {
#endif

// Expose the static manifest table for namesvc
extern const bharat_ipc_service_manifest_t namesvc_service_manifest;

#ifdef __cplusplus
}
#endif

#endif // NAMESVC_SERVICE_MANIFEST_H
