#ifndef BHARAT_POLICYMGR_H
#define BHARAT_POLICYMGR_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/system/policy.h>
#include <bharat/uapi/system/slo.h>
#include <bharat/uapi/system/policy_contract.h>

void policymgr_init(void);
void policymgr_run(void);

// Simulated effective policy payload versioned
typedef struct {
    uint32_t version;
    bharat_system_policy_t system_policy;
    bharat_slo_gates_t slo_gates;
} bharat_resolved_policy_t;

#endif // BHARAT_POLICYMGR_H
