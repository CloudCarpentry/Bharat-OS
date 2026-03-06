# Bharat-OS

<p align="center">
  <img src="assets/branding/logo-icon.png" alt="Bharat-OS logo" width="140" />
</p>

<p align="center">
  <img src="assets/branding/banner-dark.png" alt="Bharat-OS banner" />
</p>

A verification-first, capability-based microkernel OS focused on a small auditable core, strong isolation, and phased growth toward cloud and real-time profiles.

## Review Findings (What needed to change first)

- The previous README referenced directories (`drivers/`, `services/`, `profiles/`) that are not currently present in this repository.
- Core architecture and ADR navigation was spread across `docs/architecture/`, `docs/decisions/`, and `docs/adr/` with no single entry point.
- The architecture + ADR relationship (what is current v1 scope vs experimental horizon) needed one explicit navigation path for new contributors.
- Branding assets existed under `assets/branding/` but were not used in documentation.

## Vision and Scope

Bharat-OS intentionally avoids "build everything at once." We split work into:

- **v1 Core Spine (in-scope):** boot flow, memory, scheduler scaffolding, capability model, IPC, HAL foundation.
- **Deferred Research Horizon (out-of-scope for v1 correctness):** advanced compatibility layers, distributed memory/fabric features, and non-essential experimental modules.

This keeps the Trusted Computing Base (TCB) small and makes formal verification tractable.

## Current v1 Architecture Highlights

- **Verification-first microkernel core:** minimal ring-0 responsibilities.
- **Capability security model:** authority is explicit and non-ambient.
- **IPC-centric service model:** drivers and higher-level services remain isolated in user space.
- **Phased architecture bring-up:** x86_64 and riscv64 first, with arm64 support evolving through shared HAL abstractions.

For architecture details, see [`docs/architecture/README.md`](docs/architecture/README.md).

## Repository Structure (Current)

- `kernel/` — core kernel headers + source (boot, HAL, MM, IPC, scheduler scaffolding).
- `subsys/` — subsystem compatibility interfaces and manager scaffolding.
- `lib/` — shared interfaces and library-level headers.
- `tools/` — VM/bootstrap helper scripts.
- `assets/branding/` — project logo, banner, and branding guide.
- `docs/architecture/` — architecture specifications.
- `docs/decisions/` and `docs/adr/` — architectural decision records (ADRs).

## Documentation Map

- **Architecture index:** [`docs/architecture/README.md`](docs/architecture/README.md)
- **ADR index/process:** [`docs/adr/README.md`](docs/adr/README.md)
- **Accepted ADR set:** [`docs/decisions/`](docs/decisions)
- **Build & run:** [`BUILD.md`](BUILD.md)

## Implementation Status

The repository is in an **architectural scaffolding + early bring-up stage**.

Present code focuses on:

- Boot interfaces for x86_64/riscv.
- PMM/VMM scaffolding.
- Core HAL and subsystem headers.
- Experimental namespace separation under `kernel/include/experimental/` (as captured in ADR-007).

## Roadmap (Condensed)

1. **Core bring-up stability:** boot reliability, memory correctness, and baseline IPC paths.
2. **Personality and subsystem maturation:** user-space API shaping and compatibility scaffolds.
3. **Deferred expansion:** advanced fabrics, richer personalities, and broader hardware acceleration paths.

## Build and Emulation

See [`BUILD.md`](BUILD.md) for setup and cross-platform build/emulation workflow.
