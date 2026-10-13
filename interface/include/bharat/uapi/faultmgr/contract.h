#ifndef BHARAT_UAPI_FAULTMGR_CONTRACT_H
#define BHARAT_UAPI_FAULTMGR_CONTRACT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FAULTMGR_SERVICE_ID 5u

// Phase 3 — Fault Classification Engine
typedef enum {
    FAULT_CAT_CRASH = 1,
    FAULT_CAT_HEARTBEAT_FAILURE,
    FAULT_CAT_DRIVER_FAULT,
    FAULT_CAT_CORE_HEALTH,
    FAULT_CAT_IPC_FAILURE,
    FAULT_CAT_REPEATED_FAILURE
} faultmgr_category_t;

typedef enum {
    FAULT_SEV_INFO = 0,
    FAULT_SEV_WARNING,
    FAULT_SEV_ERROR,
    FAULT_SEV_CRITICAL
} faultmgr_severity_t;

// Phase 5 — Recovery Decision Engine
typedef enum {
    FAULT_REC_NO_ACTION = 0,
    FAULT_REC_RETRY,
    FAULT_REC_RESTART_SERVICE,
    FAULT_REC_ISOLATE_SERVICE,
    FAULT_REC_ENTER_DEGRADED_MODE,
    FAULT_REC_ESCALATE_TO_SUPERVISOR
} faultmgr_recovery_action_t;

typedef enum {
    FAULTMGR_OP_REPORT_FAULT = 1
} faultmgr_opcode_t;

typedef struct {
    uint32_t domain_id;
    uint32_t source_id;
    faultmgr_category_t category;
    faultmgr_severity_t severity;
    uint64_t incarnation_id;
} faultmgr_req_report_fault_t;

typedef struct {
    int32_t status;
    faultmgr_recovery_action_t action;
} faultmgr_resp_report_fault_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_FAULTMGR_CONTRACT_H
