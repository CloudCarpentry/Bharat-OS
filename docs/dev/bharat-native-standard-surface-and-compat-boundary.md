---
title: Bharat Native Standard Surface and Compatibility Boundary
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Bharat Native Standard Surface and Compatibility Boundary

## Purpose

Define the smallest stable, Bharat-native contract that all apps, services, stacks, and personalities can rely on,
without making POSIX/Linux semantics the default identity of Bharat-OS.

This policy complements (not replaces) syscall ABI freeze guidance in:

- `docs/dev/kernel-boundary-abi-freeze-plan.md`

---

## 1. Core design rule

> If an API assumes Unix semantics, it belongs in compatibility.
> If an API expresses Bharat mechanism, authority, messaging, profile, or safety intent, it belongs in native surface.

This rule is the default tie-breaker for placement disputes.

---

## 2. Three surfaces (required model)

### 2.1 Language/runtime base surface

Low-level C/runtime support that remains intentionally boring and predictable:

- memory primitives
- string primitives
- formatting core
- integer conversion
- basic time representation
- allocator hooks

### 2.2 Bharat system base surface (native)

First-class Bharat contracts exposed in `interface/uapi/` + native SDK layers:

- object handles
- capabilities and rights
- service endpoint discovery
- typed message envelopes
- shared buffer descriptors
- profile discovery
- deadline/timer intent classes
- memory class hints
- telemetry/fault contracts

### 2.3 Compatibility/personality surface

Optional surfaces for foreign semantics:

- POSIX subsets
- Linux personality APIs
- Android-oriented API adapters
- other personality projections

These must live under compatibility/personality paths, not as the default Bharat-native contract.

---

## 3. Layering and ownership

### 3.1 Kernel (mechanism-only)

Kernel exposes primitive mechanisms and enforcement points only:

- trap/syscall entry
- object/capability primitives
- scheduling and memory mechanisms
- isolation/fault containment primitives

Kernel must not absorb personality policy.

### 3.2 Services (policy + orchestration)

Services own:

- policy decisions
- lifecycle/state machines
- cross-domain orchestration
- profile-specific behavior

### 3.3 Stacks (composed subsystems)

Stacks own domain composition (e.g., storage/network/ui stacks) without redefining native core contracts.

### 3.4 UAPI boundary (external contract)

`include/bharat/interface/uapi/` is the explicit external contract boundary.
Any layout-visible type or stable constant crossing protection/domain boundaries must be defined there.

---

## 4. Bharat-native standard taxonomy

Use this taxonomy when adding public headers.

### 4.1 BH Base (freestanding)

Scope: minimal universal build/runtime substrate.

Representative headers:

- `bharat/base/types.h`
- `bharat/base/status.h`
- `bharat/base/assert.h`
- `bharat/base/time.h`
- `bharat/base/memory.h`

### 4.2 BH Core (native object + authority model)

Scope: Bharat-specific authority, object, and IPC contracts.

Representative headers:

- `bharat/object/handle.h`
- `bharat/object/capability.h`
- `bharat/object/rights.h`
- `bharat/ipc/message.h`
- `bharat/ipc/endpoint.h`
- `bharat/ipc/channel.h`
- `bharat/system/profile.h`
- `bharat/system/deadline.h`
- `bharat/system/memory_class.h`
- `bharat/system/fault_domain.h`
- `bharat/system/telemetry.h`

### 4.3 BH Services (common service clients)

Scope: stable client-side contracts for core platform services.

Representative headers:

- `bharat/service/discovery.h`
- `bharat/service/power.h`
- `bharat/service/net.h`
- `bharat/service/device.h`
- `bharat/service/storage.h`
- `bharat/service/diag.h`

### 4.4 BH Compat (personality adapters)

Scope: foreign semantic surfaces that should never define native identity.

Representative paths:

- `bharat/compat/posix/*`
- `interface/sdk/compat/linux/*`
- `interface/sdk/compat/android/*`
- `core/personalities/*`

---

## 5. Must-standardize-early primitives

Freeze these early because most subsystems depend on them:

1. status/result model
2. handle and capability type model
3. IPC message envelope shape
4. service discovery contract
5. profile descriptor/query model
6. time/deadline type model
7. memory class type model
8. fault domain + fault event model

Any change to these requires architecture review and compatibility analysis.

---

## 6. Defer/layer-later surfaces

Do not freeze these as native-core early:

- full filesystem/path abstraction set
- full buffered `FILE*` stream semantics
- POSIX process model (`fork`/`exec` assumptions)
- shell/session/env-variable assumptions
- Unix signal semantics
- Linux ioctl/procfs assumptions
- socket-only network worldview

These may exist as compat/personality layers where required.

---

## 7. API-shape policy (intent + authority visible)

Native APIs should make authority and intent explicit in signatures.

### 7.1 Authority visibility

Prefer flows like:

1. resolve service/object handle
2. present or derive capability
3. invoke typed operation

Avoid implicit-authority APIs where permission checks are opaque to callers.

### 7.2 Intent metadata

Allow optional, typed intent hints in native APIs, including:

- memory class intent (`RT`, `PACKET`, `SECURE`, etc.)
- deadline class intent (best-effort vs latency-sensitive vs hard-deadline)
- fault domain tagging
- message class intent (control/dataplane/telemetry/safety-critical)

Intent hints must be structured, versioned, and non-breaking when extended.

---

## 8. Status and error model policy

Bharat-native APIs should prefer explicit status returns over hidden global error channels.

Recommended pattern:

- return `bharat_status_t` (or typed result wrappers)
- keep errno-style adapters in compatibility surfaces only

Representative status families:

- `BHARAT_OK`
- `BHARAT_ERR_INVALID_HANDLE`
- `BHARAT_ERR_NO_CAPABILITY`
- `BHARAT_ERR_DENIED`
- `BHARAT_ERR_TIMEOUT`
- `BHARAT_ERR_UNSUPPORTED_PROFILE`
- `BHARAT_ERR_FAULT_DOMAIN_STOPPED`

Status values crossing ABI boundaries must be UAPI-defined and width-stable.

---

## 9. Placement rules for new headers

When adding any new public header:

1. classify as BH Base / BH Core / BH Services / BH Compat
2. justify placement in the PR description
3. state whether the header is ABI-visible
4. if ABI-visible, define types in `include/bharat/interface/uapi/`
5. add compatibility notes if mirroring POSIX/Linux behavior

Reject PRs that add Unix-assumption APIs directly into native core without explicit architecture approval.

---

## 10. Acceptance checklist (public contract changes)

A public contract/header change is acceptable only if all are true:

- [ ] Placement follows the core design rule (native vs compat)
- [ ] Ownership/layering is clear (kernel vs service vs stack)
- [ ] ABI-visible types are under canonical UAPI
- [ ] Status model is explicit and width-stable
- [ ] Capability/authority path is explicit where applicable
- [ ] Intent metadata is additive and version-safe where applicable
- [ ] Tests/validation updated (ABI, contract, or conformance as needed)

---

## 11. Naming guidance

Prefer naming that teaches contributors the architectural boundary:

- `bharat/base/*`
- `bharat/core/*`
- `bharat/ipc/*`
- `bharat/system/*`
- `bharat/service/*`
- `bharat/compat/*`

Avoid using `libc` as the umbrella name for the entire native contract surface.

---

## 12. Governance

This document is the architecture policy for native standard surface placement.

- ABI stability and low-level boundary enforcement remain governed by
  `docs/dev/kernel-boundary-abi-freeze-plan.md`.
- If guidance conflicts, escalate to architecture review and record resolution in both documents.
