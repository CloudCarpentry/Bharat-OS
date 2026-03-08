#ifndef BHARAT_SECURITY_AUDIT_H
#define BHARAT_SECURITY_AUDIT_H

#include <stdint.h>

typedef enum {
    BHARAT_AUDIT_EVENT_CRED_ASSIGN = 0,
    BHARAT_AUDIT_EVENT_POLICY_DENY = 1,
    BHARAT_AUDIT_EVENT_SANDBOX_APPLY = 2,
    BHARAT_AUDIT_EVENT_BOOT_MEASURE = 3
} bharat_audit_event_type_t;

typedef struct {
    uint64_t tick;
    bharat_audit_event_type_t type;
    uint32_t process_id;
    int32_t result;
    uint64_t arg0;
    uint64_t arg1;
} bharat_audit_event_t;

int bharat_audit_init(void);
int bharat_audit_record(bharat_audit_event_type_t type,
                        uint32_t process_id,
                        int32_t result,
                        uint64_t arg0,
                        uint64_t arg1);
int bharat_audit_latest(bharat_audit_event_t* out_event);

#endif // BHARAT_SECURITY_AUDIT_H
