#ifndef BHARAT_UAPI_POLICY_CONTRACT_H
#define BHARAT_UAPI_POLICY_CONTRACT_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/system/policy.h>
#include <bharat/uapi/system/slo.h>
#include <bharat/uapi/ipc/contract.h>

#ifdef __cplusplus
extern "C" {
#endif

// Policy Manager Service Interface
#define BHARAT_POLICYMGR_INTERFACE_ID 0x504F4C49 // "POLI"
#define BHARAT_POLICYMGR_INTERFACE_VERSION 1

// Opcodes
#define POLICY_OP_QUERY_SERVICE 0x100
#define POLICY_OP_QUERY_SCHED_CONSTRAINTS 0x101
#define POLICY_OP_QUERY_MEM_CONSTRAINTS 0x102
#define POLICY_OP_REPORT_SLO_VIOLATION 0x200
#define POLICY_OP_GET_SYSTEM_PROFILE 0x103

typedef enum {
    POLICY_DECISION_ALLOW = 0,
    POLICY_DECISION_DENY = 1,
    POLICY_DECISION_DEGRADED = 2,
    POLICY_DECISION_DEFERRED = 3
} bharat_policy_decision_t;

typedef struct {
    uint32_t service_id;
    uint32_t requested_caps;
} policy_req_query_service_t;

typedef struct {
    bharat_policy_decision_t decision;
    uint32_t allowed_caps;
    uint32_t max_restarts;
    uint32_t policy_version;
} policy_resp_query_service_t;

typedef struct {
    uint32_t core_id; // optional query filtering
} policy_req_query_sched_t;

typedef struct {
    bharat_slo_gates_t slo_gates;
    uint32_t default_deadline_ms;
    uint32_t policy_version;
} policy_resp_query_sched_t;

typedef struct {
    uint32_t memory_domain_id;
} policy_req_query_mem_t;

typedef struct {
    uint32_t allowed_mem_classes_mask;
    uint32_t reclaim_aggressiveness_pct;
    uint32_t policy_version;
} policy_resp_query_mem_t;

typedef struct {
    uint32_t service_id;
    bharat_slo_state_t slo_state;
    uint32_t violation_code; // system specific
} policy_req_report_slo_t;

typedef struct {
    uint32_t ack_status; // BHARAT_IPC_STATUS_OK on success
} policy_resp_report_slo_t;


typedef struct {
    // Empty request struct
    uint8_t _reserved;
} policy_req_get_profile_t;

typedef struct {
    bharat_system_policy_t system_policy;
    uint32_t policy_version;
} policy_resp_get_profile_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_POLICY_CONTRACT_H
