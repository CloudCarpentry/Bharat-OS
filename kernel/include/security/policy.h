#ifndef BHARAT_SECURITY_POLICY_H
#define BHARAT_SECURITY_POLICY_H

#include <stdint.h>
#include "security/isolation.h"

typedef enum {
    BHARAT_POLICY_MODE_PERMISSIVE = 0,
    BHARAT_POLICY_MODE_ENFORCING = 1
} bharat_policy_mode_t;

typedef struct {
    uint32_t process_id;
    bharat_isolation_class_t min_class;
    uint64_t denied_caps_mask;
    uint32_t mandatory_hooks;
} bharat_security_policy_t;

#define BHARAT_POLICY_HOOK_PROCESS_EXEC      (1U << 0)
#define BHARAT_POLICY_HOOK_MMIO_MAP          (1U << 1)
#define BHARAT_POLICY_HOOK_IPC_SEND          (1U << 2)
#define BHARAT_POLICY_HOOK_SERVICE_REGISTER  (1U << 3)

int bharat_policy_init(bharat_policy_mode_t mode);
int bharat_policy_set_process(const bharat_security_policy_t* policy);
int bharat_policy_check_operation(uint32_t process_id,
                                  uint32_t hook,
                                  bharat_isolation_class_t iso_class,
                                  uint64_t requested_caps);

#endif // BHARAT_SECURITY_POLICY_H
