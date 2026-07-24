#include "netmgr_auth_core.h"

netmgr_auth_status_t netmgr_ipc_authorize_contract(
    const netmgr_ipc_contract_t *contract,
    const netmgr_auth_context_t *ctx,
    const bh_cap_scope_t *required_scope,
    netmgr_auth_audit_t *audit_out)
{
    if (audit_out) {
        audit_out->opcode = contract->opcode;
        audit_out->caller_cap = ctx ? ctx->handle : 0;
        audit_out->required_object_type = contract->required_object_type;
        audit_out->required_rights = contract->required_rights;
        if (required_scope) {
            audit_out->required_scope = *required_scope;
        } else {
            audit_out->required_scope.kind = BH_CAP_SCOPE_GLOBAL_KIND;
            audit_out->required_scope.scope_id = 0;
        }
        audit_out->status = NETMGR_AUTH_OK;
        audit_out->audit_valid = false;
    }

    if (!ctx || !ctx->valid || ctx->handle == 0) {
        if (audit_out) {
            audit_out->status = ctx ? NETMGR_AUTH_DENY_INVALID_CAP : NETMGR_AUTH_DENY_NO_CAP;
            audit_out->audit_valid = true;
        }
        return ctx ? NETMGR_AUTH_DENY_INVALID_CAP : NETMGR_AUTH_DENY_NO_CAP;
    }

    if (ctx->revoked) {
        if (audit_out) { audit_out->status = NETMGR_AUTH_DENY_REVOKED_CAP; audit_out->audit_valid = true; }
        return NETMGR_AUTH_DENY_REVOKED_CAP;
    }

    if (ctx->stale) {
        if (audit_out) { audit_out->status = NETMGR_AUTH_DENY_STALE_CAP; audit_out->audit_valid = true; }
        return NETMGR_AUTH_DENY_STALE_CAP;
    }

    if (ctx->object_type != contract->required_object_type) {
        if (audit_out) { audit_out->status = NETMGR_AUTH_DENY_OBJECT_TYPE; audit_out->audit_valid = true; }
        return NETMGR_AUTH_DENY_OBJECT_TYPE;
    }

    if ((ctx->rights & contract->required_rights) != contract->required_rights) {
        if (audit_out) { audit_out->status = NETMGR_AUTH_DENY_RIGHTS; audit_out->audit_valid = true; }
        return NETMGR_AUTH_DENY_RIGHTS;
    }

    if (required_scope && ctx->scope.kind != BH_CAP_SCOPE_GLOBAL_KIND) {
        if (ctx->scope.kind != required_scope->kind || ctx->scope.scope_id != required_scope->scope_id) {
            if (audit_out) { audit_out->status = NETMGR_AUTH_DENY_SCOPE; audit_out->audit_valid = true; }
            return NETMGR_AUTH_DENY_SCOPE;
        }
    }

    return NETMGR_AUTH_OK;
}
