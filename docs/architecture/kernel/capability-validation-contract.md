---
title: Capability Validation Framework Contract
status: Draft
owner: Documentation Working Group
last_updated: 2026-08-13
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
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
- Production handles must contain a non-zero generation that exactly matches the entry.
- Generation-zero/raw handles fail closed. `BHARAT_ENABLE_LEGACY_CAP_TESTS` may restore
  raw-handle lookup only in explicitly identified compatibility test builds; it is OFF by
  default and must not be enabled in production target profiles.
- Explicitly requested `expected_generation` is strictly enforced.

### 5. Revocation State
Ensures that the capability is in the `CAP_STATE_LIVE` state.

### 6. Distributed CSpace Identity
Capability derivation links and cross-core delegation transactions use the fixed-width,
pointer-free `bh_cap_locator_t` identity: CSpace ID, owner core, slot, generation, and
revocation epoch. A receiver resolves a locator through its registered local CSpace and
rejects an unknown CSpace ID, wrong owner, stale slot generation, or older revocation
epoch. A capability naming a remote object authorizes only a request to its owner core;
it never authorizes direct mutation of the remote CSpace.

### 7. Process CSpace Ownership

A CSpace is a process-owned kernel object, not a CPU-local object. Its table is
allocated independently of the CPU count and the process retains the CSpace
when it is scheduled on another core. Exactly one core owns mutation authority
at a time. Each core maintains a bounded owner-local registry of the CSpaces it
currently owns so that pointer-free locators can be resolved without a global
mutable table. Creation publishes a fully initialized table; destruction first
unpublishes it, causing stale locators to fail closed, and then releases storage.
Before the heap is available, each core has one owner-local bootstrap CSpace;
normal process CSpaces use dynamically allocated storage after memory startup.

Cross-core delegation and revocation carry CSpace identity by value and execute
against the destination owner's registry. A remote core does not dereference or
mutate a CSpace table directly. Ownership transfer/sharding is a separate
transactional protocol and must publish a new generation only after the new
owner acknowledges receipt; it is not yet implemented.

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
- **CSpace transfer**: Process-owned CSpaces currently remain assigned to their
  creation core. Migration of scheduling does not itself transfer mutation
  authority; explicit CSpace ownership transfer/sharding remains future work.

## Future Evolution
- Integration into all syscall dispatch paths.
- Extension of the scope model to support security domains and services.
- Audit logging for denied capability access attempts.
