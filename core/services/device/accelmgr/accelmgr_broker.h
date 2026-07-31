#ifndef ACCELMGR_BROKER_H
#define ACCELMGR_BROKER_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/device/bharat_device_accelmgr_v2_types.h>

#define BH_HANDLE_SLOT_MASK       0xFFFFFFFFULL
#define BH_HANDLE_GENERATION_MASK 0xFFFFFFFFULL
#define BH_HANDLE_GENERATION_SHIFT 32

static inline uint64_t bh_handle_make(uint32_t index, uint32_t generation) {
    return ((uint64_t)generation << BH_HANDLE_GENERATION_SHIFT) | (index & BH_HANDLE_SLOT_MASK);
}

static inline uint32_t bh_handle_index(uint64_t handle) {
    return (uint32_t)(handle & BH_HANDLE_SLOT_MASK);
}

static inline uint32_t bh_handle_generation(uint64_t handle) {
    return (uint32_t)(handle >> BH_HANDLE_GENERATION_SHIFT);
}

// Initialize internal tables, mock devices, and telemetry
void init_accelmgr(void);

// Admin-authorized reset
int admin_reset_device(uint64_t device_handle, uint64_t admin_cap_handle);

// Telemetry injection APIs for testing
void inject_telemetry_sw_count(uint64_t val);
void inject_telemetry_cpu_fallback(uint64_t val);

// Clean, pointer-free V2 RPC handlers
int handle_GetBrokerInfo(const bharat_device_accelmgr_v2_GetBrokerInfoReq_t *req, bharat_device_accelmgr_v2_GetBrokerInfoResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_GetDeviceCount(const bharat_device_accelmgr_v2_GetDeviceCountReq_t *req, bharat_device_accelmgr_v2_GetDeviceCountResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_GetDeviceInfo(const bharat_device_accelmgr_v2_GetDeviceInfoReq_t *req, bharat_device_accelmgr_v2_GetDeviceInfoResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_RequestAdmission(const bharat_device_accelmgr_v2_RequestAdmissionReq_t *req, bharat_device_accelmgr_v2_RequestAdmissionResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_ReleaseContext(const bharat_device_accelmgr_v2_ReleaseContextReq_t *req, bharat_device_accelmgr_v2_ReleaseContextResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_CreateQueue(const bharat_device_accelmgr_v2_CreateQueueReq_t *req, bharat_device_accelmgr_v2_CreateQueueResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_DestroyQueue(const bharat_device_accelmgr_v2_DestroyQueueReq_t *req, bharat_device_accelmgr_v2_DestroyQueueResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_RegisterBuffer(const bharat_device_accelmgr_v2_RegisterBufferReq_t *req, bharat_device_accelmgr_v2_RegisterBufferResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_UnregisterBuffer(const bharat_device_accelmgr_v2_UnregisterBufferReq_t *req, bharat_device_accelmgr_v2_UnregisterBufferResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_SubmitJob(const bharat_device_accelmgr_v2_SubmitJobReq_t *req, bharat_device_accelmgr_v2_SubmitJobResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_CancelJob(const bharat_device_accelmgr_v2_CancelJobReq_t *req, bharat_device_accelmgr_v2_CancelJobResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_QueryJob(const bharat_device_accelmgr_v2_QueryJobReq_t *req, bharat_device_accelmgr_v2_QueryJobResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_QueryFence(const bharat_device_accelmgr_v2_QueryFenceReq_t *req, bharat_device_accelmgr_v2_QueryFenceResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_QueryHealth(const bharat_device_accelmgr_v2_QueryHealthReq_t *req, bharat_device_accelmgr_v2_QueryHealthResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_SetThermalConstraint(const bharat_device_accelmgr_v2_SetThermalConstraintReq_t *req, bharat_device_accelmgr_v2_SetThermalConstraintResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_SetSafeMode(const bharat_device_accelmgr_v2_SetSafeModeReq_t *req, bharat_device_accelmgr_v2_SetSafeModeResp_t *resp, uint64_t cap_handle, uint64_t client_pid);
int handle_GetTelemetrySnapshot(const bharat_device_accelmgr_v2_GetTelemetrySnapshotReq_t *req, bharat_device_accelmgr_v2_GetTelemetrySnapshotResp_t *resp, uint64_t cap_handle, uint64_t client_pid);

#define MAX_BROKER_OBJECTS 256

// Internals exposed for testing lookups
typedef enum {
    BH_ACCEL_OBJECT_NONE = 0,
    BH_ACCEL_OBJECT_DEVICE,
    BH_ACCEL_OBJECT_QUEUE,
    BH_ACCEL_OBJECT_BUFFER,
    BH_ACCEL_OBJECT_JOB,
    BH_ACCEL_OBJECT_FENCE,
} bh_accel_object_type_t;

typedef enum {
    DEV_STATE_ABSENT = 0,
    DEV_STATE_ONLINE = 1,
    DEV_STATE_DEGRADED = 2,
    DEV_STATE_QUARANTINED = 3,
} broker_dev_state_t;

typedef struct {
    uint32_t type; // bh_accel_object_type_t
    uint32_t generation;
    uint32_t active;
    uint64_t owner_pid;
    union {
        struct {
            uint32_t device_id;
            uint32_t capabilities;
            uint32_t state; // broker_dev_state_t
            char name[32];
        } device;
        struct {
            uint64_t device_handle;
            uint32_t depth;
            uint32_t priority;
            uint32_t jobs_pending;
        } queue;
        struct {
            uint64_t memory_cap_handle;
            uint64_t size;
            uint32_t flags;
        } buffer;
        struct {
            uint64_t queue_handle;
            uint64_t fence_handle;
            uint32_t opcode;
            uint32_t data_type;
            uint32_t element_count;
            uint32_t state;
            uint32_t error_code;
        } job;
        struct {
            uint32_t state;
            uint32_t error_code;
        } fence;
    } u;
} broker_object_t;

broker_object_t* lookup_object(uint64_t handle, bh_accel_object_type_t expected_type, int *err_out);

extern uint64_t g_npu_handle;
extern uint64_t g_gpu_handle;

#endif // ACCELMGR_BROKER_H
