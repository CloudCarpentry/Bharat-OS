#include "security/policy.h"
#include "security/audit.h"

#define BHARAT_MAX_POLICY_PROCS 64U

typedef struct {
    uint8_t used;
    bharat_security_policy_t policy;
} bharat_policy_slot_t;

static bharat_policy_mode_t g_policy_mode = BHARAT_POLICY_MODE_PERMISSIVE;
static bharat_policy_slot_t g_policy_slots[BHARAT_MAX_POLICY_PROCS];

static bharat_policy_slot_t* policy_find(uint32_t process_id) {
    uint32_t i;
    for (i = 0; i < BHARAT_MAX_POLICY_PROCS; ++i) {
        if (g_policy_slots[i].used && g_policy_slots[i].policy.process_id == process_id) {
            return &g_policy_slots[i];
        }
    }
    return (bharat_policy_slot_t*)0;
}

int bharat_policy_init(bharat_policy_mode_t mode) {
    uint32_t i;
    g_policy_mode = mode;
    for (i = 0; i < BHARAT_MAX_POLICY_PROCS; ++i) {
        g_policy_slots[i].used = 0U;
    }
    return 0;
}

int bharat_policy_set_process(const bharat_security_policy_t* policy) {
    uint32_t i;
    bharat_policy_slot_t* slot;

    if (!policy) {
        return -1;
    }

    slot = policy_find(policy->process_id);
    if (slot) {
        slot->policy = *policy;
        return 0;
    }

    for (i = 0; i < BHARAT_MAX_POLICY_PROCS; ++i) {
        if (!g_policy_slots[i].used) {
            g_policy_slots[i].used = 1U;
            g_policy_slots[i].policy = *policy;
            return 0;
        }
    }

    return -2;
}

int bharat_policy_check_operation(uint32_t process_id,
                                  uint32_t hook,
                                  bharat_isolation_class_t iso_class,
                                  uint64_t requested_caps) {
    bharat_policy_slot_t* slot = policy_find(process_id);
    if (!slot) {
        return 0;
    }

    if ((slot->policy.mandatory_hooks & hook) == 0U) {
        return 0;
    }

    if (iso_class < slot->policy.min_class) {
        (void)bharat_audit_record(BHARAT_AUDIT_EVENT_POLICY_DENY,
                                  process_id,
                                  -1,
                                  hook,
                                  requested_caps);
        return (g_policy_mode == BHARAT_POLICY_MODE_ENFORCING) ? -1 : 0;
    }

    if ((slot->policy.denied_caps_mask & requested_caps) != 0U) {
        (void)bharat_audit_record(BHARAT_AUDIT_EVENT_POLICY_DENY,
                                  process_id,
                                  -2,
                                  hook,
                                  requested_caps);
        return (g_policy_mode == BHARAT_POLICY_MODE_ENFORCING) ? -2 : 0;
    }

    return 0;
}
