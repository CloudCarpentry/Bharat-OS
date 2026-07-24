#ifndef BHARAT_KERNEL_SECURITY_AUDIT_H
#define BHARAT_KERNEL_SECURITY_AUDIT_H

#include <stdint.h>

typedef enum {
    BHARAT_AUDIT_EVENT_NONE = 0,
    BHARAT_AUDIT_EVENT_BOOT_MEASURE,
    BHARAT_AUDIT_EVENT_SANDBOX_APPLY,
    BHARAT_AUDIT_EVENT_CRED_ASSIGN,
    BHARAT_AUDIT_EVENT_POLICY_DENY,
} bharat_audit_event_type_t;

void bh_audit_log_event(uint32_t event_id, const char *msg);

int bharat_audit_record(bharat_audit_event_type_t type,
                        uint32_t subject_id,
                        uint32_t object_id,
                        uint64_t val1,
                        uint64_t val2);

#endif
