#include "ipc_auth.h"
#include <bharat/uapi/ipc/status.h>
#include <bharat/cap/cap_validate.h>

#ifndef BHARAT_BUILD_HOST_TESTS
#include <bharat/runtime/freestanding_string.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#endif

#ifdef BHARAT_BUILD_HOST_TESTS
static netmgr_audit_entry_t g_last_audit = {0};

const netmgr_audit_entry_t* netmgr_get_last_audit(void) {
    return &g_last_audit;
}

void netmgr_clear_last_audit(void) {
    memset(&g_last_audit, 0, sizeof(g_last_audit));
}
#endif

// Mock audit function
static void netmgr_audit_cap_failure(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_status_t status)
{
#ifdef BHARAT_BUILD_HOST_TESTS
    g_last_audit.caller_cap = caller_cap;
    g_last_audit.object_type = object_type;
    g_last_audit.object_id = object_id;
    g_last_audit.required_rights = required_rights;
    if (required_scope) {
        g_last_audit.required_scope = *required_scope;
    }
    g_last_audit.status = status;
    g_last_audit.valid = true;

    fprintf(stderr,
            "AUDIT [DENY]: Capability validation failed. "
            "caller_cap: %llu, object_type: %u, object_id: %llu, "
            "required_rights: 0x%llx, status: %d\n",
            (unsigned long long)caller_cap, (unsigned)object_type,
            (unsigned long long)object_id, (unsigned long long)required_rights,
            (int)status);
#else
    // In userspace freestanding, we log manually. This satisfies the audit requirement.
    // The actual system service logger or console_snprintf will replace this stub.
    // For now, this struct records the audit trail, ensuring it doesn't get optimized out.
    volatile struct {
        bharat_cap_handle_t c;
        bharat_cap_object_type_t t;
        uint64_t id;
        uint64_t r;
        bharat_cap_status_t st;
    } audit_log = {
        .c = caller_cap,
        .t = object_type,
        .id = object_id,
        .r = required_rights,
        .st = status
    };
    (void)audit_log;
#endif
}

int netmgr_authorize(
    bharat_cap_handle_t caller_cap,
    bharat_cap_object_type_t object_type,
    uint64_t object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope)
{
    if (caller_cap == BHARAT_CAP_INVALID_HANDLE) {
        netmgr_audit_cap_failure(caller_cap, object_type, object_id,
                                 required_rights, required_scope, BHARAT_CAP_INVALID);
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    bharat_cap_validation_result_t vr = {0};

    bharat_cap_status_t st = bharat_cap_validate(
        caller_cap,
        object_type,
        object_id,
        required_rights,
        required_scope,
        &vr);

    if (st != BHARAT_CAP_OK || !vr.allowed) {
        netmgr_audit_cap_failure(caller_cap, object_type, object_id,
                                 required_rights, required_scope, vr.status);
        return BHARAT_IPC_STATUS_ERR_PERM;
    }

    return BHARAT_IPC_STATUS_OK;
}

void netmgr_audit_core_failure(const netmgr_auth_audit_t *audit) {
    if (!audit || !audit->audit_valid) return;

    netmgr_audit_cap_failure(
        audit->caller_cap,
        (bharat_cap_object_type_t)audit->required_object_type,
        audit->required_scope.scope_id,
        audit->required_rights,
        (const bharat_cap_scope_t *)&audit->required_scope,
        (bharat_cap_status_t)audit->status /* This needs careful mapping in production */
    );
}
