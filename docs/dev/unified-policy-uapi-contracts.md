# Unified Policy UAPI Contracts (uapi/policy)

## Scope

This document defines the baseline **Unified Policy UAPI** contracts that centralize profile/capability policy resolution and measurable SLO gate evaluation without moving policy logic into the kernel.

Layer ownership remains aligned with Bharat-OS principles:

- Kernel: mechanism only (scheduler primitives, IPC primitives, memory classes, power/fault hooks).
- Services: policy and orchestration (`policymgr`, `telemetrymgr`, `schedmgr`, `powermgr`, `safetymgr`).
- UAPI: stable shared contracts so policy decisions are explicit and auditable.

## New UAPI headers

- `include/bharat/uapi/policy/contract.h`
  - policy opcodes for boot snapshot resolution, runtime updates, and SLO gate workflows
  - snapshot schema capturing profile/hardware/tier/subsystem state
  - first-class SLO domains and enforcement action vocabulary
- `include/bharat/uapi/policy/status.h`
  - policy-level status aliases and policy-specific failures (profile mismatch, unsupported tier, SLO gate failure)

## Contract model

### 1) Boot-time policy snapshot

`BHARAT_POLICY_OP_RESOLVE_BOOT_SNAPSHOT` returns a `bharat_policy_snapshot_t` containing:

- `profile_mask`
- `hw_capability_flags`
- protection model (`MPU_ONLY`, `MMU_LITE`, `MMU_FULL`)
- realization tier (`STATIC`, `LIGHTWEIGHT`, `FULL`)
- enabled subsystem bitmask
- generation + effective timestamp

This allows one canonical policy realization at boot instead of independent scheduler/IPC/memory fragments.

### 2) Runtime updates (tier-scaled)

`BHARAT_POLICY_OP_APPLY_RUNTIME_UPDATE` introduces generation-checked updates:

- caller provides `expected_generation`
- policymgr either applies a new snapshot or rejects stale/unsupported updates

Tiny targets can keep this path dormant (Tier A), while Tier B/C systems can use it for controlled runtime adaptation.

### 3) SLO gate flow

SLO contracts are explicit and measurable:

- `BHARAT_POLICY_OP_REPORT_SLO_SAMPLE`
- `BHARAT_POLICY_OP_EVAL_SLO_GATES`
- standardized domains for initial rollout:
  - scheduler latency
  - IPC queue pressure
  - memory pressure
  - watchdog heartbeat health
  - thermal violations

Evaluation responses carry both gate status and enforcement action, allowing deterministic orchestration by peer services.

## Why this shape

This avoids duplicate policy models while keeping implementation scalable:

- Tier A (static): compile/boot-resolved snapshot, minimal runtime churn
- Tier B (lightweight): event-driven updates and threshold gates
- Tier C (full): telemetry-integrated dynamic policy plane

The UAPI surface is shared across tiers; only operational richness changes.
