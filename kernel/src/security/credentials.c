#include "security/credentials.h"
#include "security/audit.h"

#define BHARAT_MAX_CRED_PROCS 64U

typedef struct {
    uint8_t used;
    uint32_t process_id;
    bharat_credentials_t cred;
} bharat_cred_slot_t;

static bharat_cred_slot_t g_cred_slots[BHARAT_MAX_CRED_PROCS];

static bharat_cred_slot_t* cred_find_slot(uint32_t process_id) {
    uint32_t i;
    for (i = 0; i < BHARAT_MAX_CRED_PROCS; ++i) {
        if (g_cred_slots[i].used && g_cred_slots[i].process_id == process_id) {
            return &g_cred_slots[i];
        }
    }
    return (bharat_cred_slot_t*)0;
}

static bharat_cred_slot_t* cred_alloc_slot(uint32_t process_id) {
    uint32_t i;
    bharat_cred_slot_t* existing = cred_find_slot(process_id);
    if (existing) {
        return existing;
    }

    for (i = 0; i < BHARAT_MAX_CRED_PROCS; ++i) {
        if (!g_cred_slots[i].used) {
            g_cred_slots[i].used = 1U;
            g_cred_slots[i].process_id = process_id;
            return &g_cred_slots[i];
        }
    }
    return (bharat_cred_slot_t*)0;
}

int bharat_credentials_init(void) {
    uint32_t i;
    for (i = 0; i < BHARAT_MAX_CRED_PROCS; ++i) {
        g_cred_slots[i].used = 0U;
        g_cred_slots[i].process_id = 0U;
    }
    return 0;
}

int bharat_credentials_assign_process(uint32_t process_id,
                                     const bharat_credentials_t* cred) {
    uint32_t i;
    bharat_cred_slot_t* slot;

    if (!cred || cred->supp_group_count > BHARAT_MAX_SUPP_GROUPS) {
        return -1;
    }

    slot = cred_alloc_slot(process_id);
    if (!slot) {
        return -2;
    }

    slot->cred = *cred;
    for (i = cred->supp_group_count; i < BHARAT_MAX_SUPP_GROUPS; ++i) {
        slot->cred.supp_groups[i] = 0U;
    }

    (void)bharat_audit_record(BHARAT_AUDIT_EVENT_CRED_ASSIGN,
                              process_id,
                              0,
                              cred->uid,
                              cred->capability_mask);

    return 0;
}

int bharat_credentials_get_process(uint32_t process_id,
                                  bharat_credentials_t* out_cred) {
    bharat_cred_slot_t* slot;
    if (!out_cred) {
        return -1;
    }

    slot = cred_find_slot(process_id);
    if (!slot) {
        return -2;
    }

    *out_cred = slot->cred;
    return 0;
}

int bharat_credentials_check_capability(uint32_t process_id,
                                        uint64_t required_capability) {
    bharat_credentials_t cred;
    int rc = bharat_credentials_get_process(process_id, &cred);
    if (rc != 0) {
        return rc;
    }

    if ((cred.capability_mask & required_capability) != required_capability) {
        return -3;
    }

    return 0;
}
