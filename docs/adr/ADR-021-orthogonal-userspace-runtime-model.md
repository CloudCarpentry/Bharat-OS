---
title: Orthogonal userspace runtime model and canonical root selection
status: Accepted
owner: Userspace Architecture Team
---

# ADR-021: Orthogonal userspace runtime model and canonical root selection

## Context

Packaging previously inferred the root task from the combined RT and MPU-only
configuration. That coupled lifecycle policy to execution and memory-protection
dimensions and made architecture bring-up depend on a service graph even when a
single userspace program was sufficient.

## Decision

Targets may select exactly one `userspace.runtime_model`: `direct`, `static`,
`light`, or `full`. Targets that omit it resolve explicitly to `full` during the
migration. The target resolver is the authority that maps these values to
`user_smoke`, `rt-supervisor`, `init-lite`, and the existing `init`, respectively.
Architecture, device, execution, and memory-protection profiles do not supply or
override this value.

The packager emits the selected executable as the existing single root boot
module. It does not create a package manager, dependency solver, or general boot
bundle. All architectures consume the same module format and root launcher.

`bharat_user_startup_t` remains the generic, unchanged startup ABI. For the root
task only, its extension flag identifies a versioned `bh_root_launch_info_t`
stored immediately after the generic structure. The extension contains only
fixed-width, by-value identifiers and immutable launch policy. It contains no
pointers or capabilities and does not authorize spawning. A future bootstrap
capability contract requires a separate security review.

## Invariants and failure behavior

- The resolver defaults a missing model to `full`; an unknown, misplaced, or
  duplicated model fails schema validation.
- Root selection happens once in common tooling, never in an ISA implementation.
- Kernel scheduler, VM, syscall, capability, and IPC mechanisms do not branch on
  runtime model. The loader only publishes the selected numeric value.
- The root validates extension version, size, model, and boot-session binding.
- Missing compiled root payloads fail packaging; no synthetic image is emitted.
- DIRECT has no service-graph completion requirement. STATIC, LIGHT, and FULL
  retain distinct lifecycle evidence.
- The FULL root attempts supervisor handoff only when the resolved declarative
  service graph contains `servicemgr`. Manager-less graphs remain valid with
  `init` as the quiescent lifecycle authority; hardware and execution profiles
  never select a different kernel root path.
- A required userspace bootstrap failure unwinds successfully activated
  services in reverse order through explicit compensating callbacks. Optional
  failures retain activated dependencies and produce a degraded outcome.
- Supervisor handoff retains the versioned `BEGIN` / `SERVICE` / `COMMIT`
  transaction, uses bounded profile-policy timeouts/retries, and relies on
  receiver idempotency when an ambiguous timeout causes replay.
- Early framebuffer ownership uses capability-authorized begin/commit/abort.
  Commit alone quiesces framebuffer console sinks; abort and every failed
  transfer preserve kernel display output, while serial and emergency sinks
  are never disabled. The framebuffer capability is granted into the receiving
  process cspace and is not returned from a destroyed temporary table.

## Consequences

Any architecture/profile combination may request any model that its independently
validated mechanisms support. This ADR does not claim that every combination
currently reaches user mode, and it does not authorize trap, context-switch,
MMU, MPU, SMP, syscall, or capability redesign.
