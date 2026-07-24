#ifndef BHARAT_BOOT_SECURITY_H
#define BHARAT_BOOT_SECURITY_H

#include "boot_contract.h"
#include "boot_info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOOT_SEC_DECISION_ALLOW = 0,
    BOOT_SEC_DECISION_DENY,
    BOOT_SEC_DECISION_WARN_AND_ALLOW
} boot_security_decision_t;

// Evaluate if the current boot_info security posture is acceptable
// for the selected boot profile and mode.
int boot_security_evaluate(const boot_info_t *bi, boot_security_decision_t *out_decision);

// Check if a specific mode is permitted under the current security constraints.
bool boot_security_allows_mode(const boot_info_t *bi, boot_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_SECURITY_H
