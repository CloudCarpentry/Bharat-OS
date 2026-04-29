#ifndef NETMGR_AUTH_CORE_H
#define NETMGR_AUTH_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Netmgr Authorization Core
 *
 * This header defines the pure data structures for network manager
 * capability contracts and authorization contexts. It is designed
 * to be host-testable without pulling in service runtime dependencies.
 */

/* Minimal subset of capability types needed for the core auth logic */
typedef uint64_t bh_cap_handle_t;
typedef uint32_t bh_cap_obj_type_t;

typedef enum {
    BH_CAP_SCOPE_GLOBAL_KIND = 0,
    BH_CAP_SCOPE_OBJECT_KIND,
    BH_CAP_SCOPE_NAMESPACE_KIND,
    BH_CAP_SCOPE_SERVICE_KIND,
} bh_cap_scope_kind_t;

typedef struct {
    bh_cap_scope_kind_t kind;
    uint64_t scope_id;
} bh_cap_scope_t;

/* Status codes for authorization result */
typedef enum {
    NETMGR_AUTH_OK = 0,
    NETMGR_AUTH_DENY_INVALID_CAP,
    NETMGR_AUTH_DENY_STALE_CAP,
    NETMGR_AUTH_DENY_REVOKED_CAP,
    NETMGR_AUTH_DENY_RIGHTS,
    NETMGR_AUTH_DENY_OBJECT_TYPE,
    NETMGR_AUTH_DENY_SCOPE,
    NETMGR_AUTH_DENY_NO_CAP
} netmgr_auth_status_t;

/* Contract for a specific netmgr IPC opcode */
typedef struct {
    uint32_t opcode;
    const char *name;
    bh_cap_obj_type_t required_object_type;
    uint64_t required_rights;
    size_t min_request_size;
    size_t max_request_size;
    bool implemented;
} netmgr_ipc_contract_t;

/* Context representing the caller's provided capability */
typedef struct {
    bh_cap_handle_t handle;
    bh_cap_obj_type_t object_type;
    uint64_t rights;
    bh_cap_scope_t scope;
    bool valid;
    bool stale;
    bool revoked;
} netmgr_auth_context_t;

/* Audit record for authorization failures */
typedef struct {
    uint32_t opcode;
    bh_cap_handle_t caller_cap;
    bh_cap_obj_type_t required_object_type;
    uint64_t required_rights;
    bh_cap_scope_t required_scope;
    netmgr_auth_status_t status;
    bool audit_valid;
} netmgr_auth_audit_t;

/**
 * @brief Authorize a request based on a contract and caller context.
 *
 * @param contract The contract for the opcode.
 * @param ctx The authorization context (caller's capability info).
 * @param required_scope The specific scope required for this request.
 * @param audit_out Output for audit information on failure.
 * @return NETMGR_AUTH_OK on success, or an error status.
 */
netmgr_auth_status_t netmgr_ipc_authorize_contract(
    const netmgr_ipc_contract_t *contract,
    const netmgr_auth_context_t *ctx,
    const bh_cap_scope_t *required_scope,
    netmgr_auth_audit_t *audit_out);

#endif // NETMGR_AUTH_CORE_H
