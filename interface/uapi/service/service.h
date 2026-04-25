#ifndef BHARAT_UAPI_SERVICE_H
#define BHARAT_UAPI_SERVICE_H

#include <stdint.h>
#include <interface/uapi/ipc/ipc.h>

struct bh_service_info {
    char name[64];
    uint32_t version;
    bh_endpoint_t endpoint;
};

#endif /* BHARAT_UAPI_SERVICE_H */
