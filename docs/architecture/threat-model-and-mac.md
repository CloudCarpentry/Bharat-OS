# Bharat-OS Threat Model and Mandatory Access Control (MAC) Baseline

This document defines the minimum security model expected for production Bharat-OS kernels.

## Threat model

### Assets

- Kernel integrity (text, data, control flow).
- Capability table integrity and rights boundaries.
- Task isolation for memory mappings and IPC channels.
- Driver isolation and MMIO mapping boundaries.

### Adversaries

- Compromised user-space task attempting privilege escalation.
- Malicious or buggy driver task trying to access unauthorized MMIO or DMA surfaces.
- Faulty kernel client abusing IPC and memory APIs with malformed parameters.

### Security objectives

- **Least privilege**: all privileged operations gated by capabilities.
- **Complete mediation**: every map/unmap/send/receive path validates rights and bounds.
- **Fail closed**: invalid arguments return explicit errors and do not mutate state.
- **Containment**: failures in a task/driver domain must not corrupt global kernel state.

## MAC baseline policy

### Labels and domains

Each task belongs to a security domain label:

- `domain::kernel`
- `domain::driver::<class>`
- `domain::service::<name>`
- `domain::app::<name>`

### Enforcement rules

1. Capability invocation is denied unless the capability label dominates the caller's domain policy.
2. Device MMIO mapping requires both capability rights and an allowed `(domain, device_id)` pair.
3. Endpoint IPC is denied unless sender and receiver domains match an allow-rule in policy.
4. Capability delegation can only reduce rights (never amplify rights or widen domain access).

### Audit requirements

The kernel should emit auditable events for:

- denied capability invocation,
- denied MMIO mapping,
- denied IPC delivery,
- repeated malformed API usage (potential abuse).

## Implementation checklist

- [ ] Define per-task domain labels in scheduler/task metadata.
- [ ] Add policy evaluator on capability invocation and IPC send paths.
- [ ] Add `(device_id, domain)` checks in zero-copy/MMIO mapping paths.
- [ ] Add structured audit event record format and sink.
