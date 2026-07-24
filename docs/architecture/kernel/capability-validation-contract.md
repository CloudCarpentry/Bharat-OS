# Capability Validation Framework Contract

## Overview
The Capability Validation Framework provides a reusable, strict, and fail-closed validation layer for Bharat-OS capabilities. It is designed to be the canonical entry point for checking capability validity at security boundaries such as system calls and IPC entry points.

## Goals
- **Fail-Closed**: Any validation failure results in immediate denial of access.
- **Canonical**: Centralized validation logic to prevent ad-hoc check bypasses.
- **Strict**: Validates object type, rights, scope, and generation.
- **Reusable**: Independent of specific syscall or IPC logic.

## Validation Components

### 1. Object Type Validation
Ensures that the capability being used matches the expected kernel object type (e.g., Endpoint, Memory, Thread).

### 2. Rights Validation
Verifies that the capability possesses all the rights required for the requested operation.

### 3. Scope Validation
Validates that the requester has the authority to use the capability. In Phase K3-S0, this is primarily based on the `owner_pid` of the capability table.

### 4. Generation & Stale Handle Validation
Prevents "use-after-revocation" or "use-after-reallocation" by checking generation numbers.
- Handles containing a non-zero generation are checked against the entry.
- Explicitly requested `expected_generation` is strictly enforced.

### 5. Revocation State
Ensures that the capability is in the `CAP_STATE_LIVE` state.

## API Specification

```c
typedef struct cap_validation_request {
    uint32_t cap_id;                 // The capability handle/ID to validate
    cap_type_t expected_object_type; // The required type, or CAP_TYPE_NONE to skip
    cap_rights_mask_t required_rights;// Bitmask of required rights
    uint32_t requester_pid;          // PID of the requester for scope check
    uint64_t expected_generation;    // Optional strict generation check
} cap_validation_request_t;

kstatus_t cap_validate_ex(capability_table_t *table,
                          const cap_validation_request_t *req,
                          capability_entry_t **out_entry);
```

## Error Codes
- `K_OK`: Validation successful.
- `K_ERR_NOT_FOUND`: Capability ID does not exist in the table.
- `K_ERR_CAP_WRONG_TYPE`: Capability exists but is of the wrong type.
- `K_ERR_CAP_DENIED`: Missing required rights or scope mismatch.
- `K_ERR_CAP_STALE`: Generation mismatch (stale handle or stale request).
- `K_ERR_CAP_REVOKED`: Capability has been revoked and is no longer live.
- `K_ERR_INVALID_ARG`: Null table or request pointer.

## Current Limitations (Phase K3-S0)
- **Rollout**: Not yet wired into every syscall boundary.
- **Scope Model**: Minimal security-domain model (PID-based only).
- **Revocation**: Distributed revocation semantics are still being matured.

## Future Evolution
- Integration into all syscall dispatch paths.
- Extension of the scope model to support security domains and services.
- Audit logging for denied capability access attempts.
