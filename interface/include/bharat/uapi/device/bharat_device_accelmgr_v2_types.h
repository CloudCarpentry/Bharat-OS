#pragma once
#include <stdint.h>

typedef struct {
    uint32_t dummy;
} bharat_device_accelmgr_v2_GetBrokerInfoReq_t;

typedef struct {
    uint32_t status;
    uint32_t version;
} bharat_device_accelmgr_v2_GetBrokerInfoResp_t;

typedef struct {
    uint32_t dummy;
} bharat_device_accelmgr_v2_GetDeviceCountReq_t;

typedef struct {
    uint32_t status;
    uint32_t count;
} bharat_device_accelmgr_v2_GetDeviceCountResp_t;

typedef struct {
    uint32_t index;
} bharat_device_accelmgr_v2_GetDeviceInfoReq_t;

typedef struct {
    uint32_t status;
    uint32_t device_id;
    uint32_t capabilities;
    uint32_t state;
    struct { uint32_t len; char data[32]; } name;
} bharat_device_accelmgr_v2_GetDeviceInfoResp_t;

typedef struct {
    uint32_t feature_class;
    uint32_t requested_qos;
} bharat_device_accelmgr_v2_RequestAdmissionReq_t;

typedef struct {
    uint32_t status;
    uint32_t granted_qos;
    uint64_t context_handle;
} bharat_device_accelmgr_v2_RequestAdmissionResp_t;

typedef struct {
    uint64_t context_handle;
} bharat_device_accelmgr_v2_ReleaseContextReq_t;

typedef struct {
    uint32_t status;
} bharat_device_accelmgr_v2_ReleaseContextResp_t;

typedef struct {
    uint64_t context_handle;
    uint64_t device_handle;
    uint32_t depth;
    uint32_t priority;
} bharat_device_accelmgr_v2_CreateQueueReq_t;

typedef struct {
    uint32_t status;
    uint64_t queue_handle;
} bharat_device_accelmgr_v2_CreateQueueResp_t;

typedef struct {
    uint64_t queue_handle;
} bharat_device_accelmgr_v2_DestroyQueueReq_t;

typedef struct {
    uint32_t status;
} bharat_device_accelmgr_v2_DestroyQueueResp_t;

typedef struct {
    uint64_t memory_cap_handle;
    uint64_t size;
    uint32_t flags;
} bharat_device_accelmgr_v2_RegisterBufferReq_t;

typedef struct {
    uint32_t status;
    uint64_t buffer_handle;
} bharat_device_accelmgr_v2_RegisterBufferResp_t;

typedef struct {
    uint64_t buffer_handle;
} bharat_device_accelmgr_v2_UnregisterBufferReq_t;

typedef struct {
    uint32_t status;
} bharat_device_accelmgr_v2_UnregisterBufferResp_t;

typedef struct {
    uint64_t queue_handle;
    uint64_t input_buffer_handle;
    uint64_t input_offset;
    uint64_t input_length;
    uint64_t output_buffer_handle;
    uint64_t output_offset;
    uint64_t output_length;
    uint32_t opcode;
    uint32_t data_type;
    uint32_t element_count;
    uint64_t dependency_fence_handle;
    uint32_t flags;
} bharat_device_accelmgr_v2_SubmitJobReq_t;

typedef struct {
    uint32_t status;
    uint64_t job_handle;
    uint64_t fence_handle;
} bharat_device_accelmgr_v2_SubmitJobResp_t;

typedef struct {
    uint64_t job_handle;
} bharat_device_accelmgr_v2_CancelJobReq_t;

typedef struct {
    uint32_t status;
} bharat_device_accelmgr_v2_CancelJobResp_t;

typedef struct {
    uint64_t job_handle;
} bharat_device_accelmgr_v2_QueryJobReq_t;

typedef struct {
    uint32_t status;
    uint32_t state;
    uint32_t error_code;
} bharat_device_accelmgr_v2_QueryJobResp_t;

typedef struct {
    uint64_t fence_handle;
} bharat_device_accelmgr_v2_QueryFenceReq_t;

typedef struct {
    uint32_t status;
    uint32_t state;
    uint32_t error_code;
} bharat_device_accelmgr_v2_QueryFenceResp_t;

typedef struct {
    uint64_t device_handle;
} bharat_device_accelmgr_v2_QueryHealthReq_t;

typedef struct {
    uint32_t status;
    uint32_t state;
    uint32_t temperature_c;
    uint32_t utilization_pct;
} bharat_device_accelmgr_v2_QueryHealthResp_t;

typedef struct {
    uint64_t device_handle;
    uint32_t max_power_mw;
} bharat_device_accelmgr_v2_SetThermalConstraintReq_t;

typedef struct {
    uint32_t status;
} bharat_device_accelmgr_v2_SetThermalConstraintResp_t;

typedef struct {
    uint32_t enable_safe_mode;
} bharat_device_accelmgr_v2_SetSafeModeReq_t;

typedef struct {
    uint32_t status;
} bharat_device_accelmgr_v2_SetSafeModeResp_t;

typedef struct {
    uint32_t dummy;
} bharat_device_accelmgr_v2_GetTelemetrySnapshotReq_t;

typedef struct {
    uint32_t status;
    uint64_t jobs_submitted;
    uint64_t jobs_completed;
    uint64_t jobs_failed;
    uint64_t jobs_cancelled;
    uint64_t queue_depth;
    uint64_t execution_latency_us;
    uint64_t hw_backend_selected;
    uint64_t sw_backend_selected;
    uint64_t cpu_fallback_count;
    uint64_t capability_denials;
    uint64_t buffer_reg_failures;
    uint64_t device_resets;
    uint64_t device_quarantines;
} bharat_device_accelmgr_v2_GetTelemetrySnapshotResp_t;
