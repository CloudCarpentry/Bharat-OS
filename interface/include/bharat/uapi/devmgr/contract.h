#ifndef BHARAT_UAPI_DEVMGR_CONTRACT_H
#define BHARAT_UAPI_DEVMGR_CONTRACT_H

#include <stdint.h>
#include <bharat/uapi/services/service_ids.h>
#include <bharat/uapi/ipc/contract.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file contract.h
 * @brief IPC contract for the Device Manager (devmgr) service.
 */

#define DEVMGR_SERVICE_ID BHARAT_SERVICE_DEVMGR

/* Opcodes */
enum {
    DEVMGR_OP_HOTPLUG_DEVICE    = 1001,
    DEVMGR_OP_BIND_DRIVER       = 1002,
    DEVMGR_OP_REMOVE_DEVICE     = 1003,
    DEVMGR_OP_RESET_DEVICE      = 1004,
    DEVMGR_OP_QUERY_DEVICE      = 1005,
};

/* Structs */
typedef struct {
    uint32_t bus;
    uint32_t device_id;
    uint32_t vendor_id;
    uint32_t class_id;
    char name[32];
    char compatible[32];
} devmgr_req_hotplug_t;

typedef struct {
    int32_t status;
    uint32_t device_registry_id;
} devmgr_resp_hotplug_t;

typedef struct {
    uint32_t device_registry_id;
    char driver_name[32];
} devmgr_req_bind_t;

typedef struct {
    uint32_t device_registry_id;
} devmgr_req_remove_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_DEVMGR_CONTRACT_H
