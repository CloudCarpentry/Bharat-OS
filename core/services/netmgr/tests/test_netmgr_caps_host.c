#include "include/netmgr_host_test_compat.h"
#include "../include/netmgr_auth_core.h"
#include <bharat/network/netmgr_ipc.h>
#include "../include/ipc_auth.h"
#include <bharat/cap/cap_validate.h>
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("Starting Netmgr Core Auth Host Tests...\n");

    /* Test Setup */
    netmgr_ipc_contract_t contract_admin = {
        .opcode = NETMGR_OP_CREATE_IFACE,
        .name = "CREATE_IFACE",
        .required_object_type = BHARAT_CAP_OBJ_SERVICE,
        .required_rights = BHARAT_CAP_RIGHT_NET_ADMIN,
        .implemented = true
    };

    netmgr_ipc_contract_t contract_stats = {
        .opcode = NETMGR_OP_QUERY_STATS,
        .name = "QUERY_STATS",
        .required_object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .required_rights = BHARAT_CAP_RIGHT_NET_READ_STATS,
        .implemented = true
    };

    netmgr_auth_context_t ctx_admin = {
        .handle = 1,
        .object_type = BHARAT_CAP_OBJ_SERVICE,
        .rights = BHARAT_CAP_RIGHT_NET_ADMIN,
        .scope = { .kind = BH_CAP_SCOPE_GLOBAL_KIND, .scope_id = 0 },
        .valid = true,
        .revoked = false,
        .stale = false
    };

    netmgr_auth_context_t ctx_stats_if5 = {
        .handle = 2,
        .object_type = BHARAT_CAP_OBJ_NET_IFACE,
        .rights = BHARAT_CAP_RIGHT_NET_READ_STATS,
        .scope = { .kind = BH_CAP_SCOPE_OBJECT_KIND, .scope_id = 5 },
        .valid = true,
        .revoked = false,
        .stale = false
    };

    netmgr_auth_audit_t audit = {0};
    bh_cap_scope_t req_scope_if5 = { .kind = BH_CAP_SCOPE_OBJECT_KIND, .scope_id = 5 };
    bh_cap_scope_t req_scope_if6 = { .kind = BH_CAP_SCOPE_OBJECT_KIND, .scope_id = 6 };

    /* Test 1: Valid Global Admin Cap */
    printf("Test 1: Valid Global Admin Cap\n");
    assert(netmgr_ipc_authorize_contract(&contract_admin, &ctx_admin, NULL, &audit) == NETMGR_AUTH_OK);

    /* Test 2: Missing Capability */
    printf("Test 2: Missing Capability\n");
    assert(netmgr_ipc_authorize_contract(&contract_admin, NULL, NULL, &audit) == NETMGR_AUTH_DENY_NO_CAP);
    assert(audit.status == NETMGR_AUTH_DENY_NO_CAP);
    assert(audit.opcode == NETMGR_OP_CREATE_IFACE);

    /* Test 3: Missing Rights */
    printf("Test 3: Missing Rights\n");
    assert(netmgr_ipc_authorize_contract(&contract_admin, &ctx_stats_if5, NULL, &audit) == NETMGR_AUTH_DENY_OBJECT_TYPE);

    /* Test 4: Valid Scoped Cap */
    printf("Test 4: Valid Scoped Cap\n");
    assert(netmgr_ipc_authorize_contract(&contract_stats, &ctx_stats_if5, &req_scope_if5, &audit) == NETMGR_AUTH_OK);

    /* Test 5: Out of Scope */
    printf("Test 5: Out of Scope\n");
    assert(netmgr_ipc_authorize_contract(&contract_stats, &ctx_stats_if5, &req_scope_if6, &audit) == NETMGR_AUTH_DENY_SCOPE);
    assert(audit.status == NETMGR_AUTH_DENY_SCOPE);

    /* Test 6: Revoked Cap */
    printf("Test 6: Revoked Cap\n");
    ctx_admin.revoked = true;
    assert(netmgr_ipc_authorize_contract(&contract_admin, &ctx_admin, NULL, &audit) == NETMGR_AUTH_DENY_REVOKED_CAP);
    ctx_admin.revoked = false;

    /* Test 7: Stale Cap */
    printf("Test 7: Stale Cap\n");
    ctx_admin.stale = true;
    assert(netmgr_ipc_authorize_contract(&contract_admin, &ctx_admin, NULL, &audit) == NETMGR_AUTH_DENY_STALE_CAP);
    ctx_admin.stale = false;

    printf("Netmgr Core Auth Host Tests Passed Successfully!\n");
    return 0;
}
