---
name: bharat-os-capability-security
description: Apply when working on capabilities, CSpace, IPC authorization, rights verification, revocation, generation checks, or security trust boundaries.
---

# Bharat-OS Capability Security Skill

## Foundational Principle

Bharat-OS is an object-capability operating system. Possession of an unforgeable capability token is the sole authority for invoking any operation or accessing any resource.

- **Deny by default**: An operation lacking explicit authority is rejected.
- **Fail closed**: Any unexpected condition, invalid handle, or capability check failure results in immediate access denial (`BH_STATUS_ERR_PERMISSION_DENIED` or `BH_STATUS_ERR_INVALID_CAPABILITY`).

## Capability Verification Model

Every invocation of a capability must validate five dimensions:

1. **Object Type**: The capability references an object matching the requested operation (e.g., `BH_CAP_TYPE_THREAD`, `BH_CAP_TYPE_ASPACE`, `BH_CAP_TYPE_IPC_ENDPOINT`).
2. **Rights Mask**: The capability contains the necessary permission bits for the requested action (e.g., `BH_CAP_RIGHT_READ`, `BH_CAP_RIGHT_WRITE`, `BH_CAP_RIGHT_GRANT`, `BH_CAP_RIGHT_MAP`).
3. **Scope & Namespace**: The capability is valid within the calling process's CSpace (Capability Space).
4. **Liveness & Generation**: The capability's generation counter matches the target object's current generation. If the object was deleted and recreated (ABA situation), the generation check fails.
5. **Ownership & Revocation**: The capability has not been marked revoked in the CSpace revocation tree.

## Security Boundaries & Rules

1. **Usercopy Boundary Before Capability Validation (TOCTOU Prevention)**:
   - When a syscall receives userland memory containing capability handles or descriptor tables, **never validate capabilities while they remain in user memory**.
   - Copy the structure into trusted kernel memory first via `bh_usercopy_from_user()`, then validate and look up capabilities in the kernel copy.
2. **No Capability Bypasses**:
   - Never allow hardcoded "superuser", "root", or synthetic bypass IDs to circumvent capability checks.
   - IPC endpoints, devmgr, vm_manager, and process_manager must authorize every incoming request against the caller's badge / capability.
3. **Revocation Semantics**:
   - Support both single capability invalidation and recursive subtree revocation.
   - Revocation must be atomic with respect to capability lookups.
   - After revocation completes, any subsequent operation using that handle must fail closed.
4. **IPC Authorization Coverage**:
   - Every IPC dispatch handler in the microkernel and services must explicitly enforce capability permissions for each sub-operation.
   - Automated linters check that every IPC dispatch route contains an explicit authorization check.

## Verification Checklist

- [ ] Capability type check implemented and verified.
- [ ] Rights bitmask check enforced with exact bit constants (`BH_CAP_RIGHT_*`).
- [ ] Generation counter checked against object generation.
- [ ] Usercopy completed before reading embedded capability handles.
- [ ] Negative tests included:
  - Invoking with wrong capability type (must return `ERR_INVALID_CAPABILITY`).
  - Invoking with missing right (must return `ERR_PERMISSION_DENIED`).
  - Invoking with stale generation / revoked capability (must return `ERR_STALE_HANDLE`).
  - Attempting TOCTOU userland pointer mutation during call.
