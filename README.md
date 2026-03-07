# Bharat-OS

<p align="center">
  <img src="assets/branding/logo-icon.png" alt="Bharat-OS logo" width="140" />
</p>

<p align="center">
  <img src="assets/branding/banner-dark.png" alt="Bharat-OS banner" />
</p>

<p align="center"><em>Official Bharat-OS logo and banner assets</em></p>

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

## Architecture Design (v1 Blueprint)

```mermaid
flowchart TD
    subgraph Userspace["User Space / Services"]
        direction LR
        D[Drivers] --- F[Filesystems] --- N[Network Stack] --- R[Runtime Services]
    end

    subgraph Microkernel["Microkernel (Trusted Core)"]
        direction LR
        S[Scheduling] --- I[IPC Routing] --- C[Capability Checks] --- V[VM/PMM API]
    end

    subgraph HAL["Hardware Abstraction Layer (HAL)"]
        direction LR
        Int[Interrupts] --- T[Timer] --- M[MMU] --- Plat[Device/Platform Glue]
    end

    subgraph Hardware["Hardware Platforms"]
        direction LR
        X86[x86_64] --- RISCV[riscv64] -.-> ARM[arm64 planned]
    end

    Userspace <-->|Capability-guarded IPC| Microkernel
    Microkernel <-->|HAL interfaces| HAL
    HAL <--> Hardware
```

Design principles represented above:

- **Small trusted core:** keep privileged code minimal for easier auditing and verification.
- **Capability-mediated communication:** every cross-boundary interaction is explicit and enforceable.
- **Portability by HAL:** architecture-specific details are isolated so higher layers remain stable.
- **Service isolation:** drivers and services stay in user space and interact via IPC contracts.

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

## v1 Kernel Functionality Tiers

For scope control, Bharat-OS classifies kernel functionality into three explicit tiers:

- **Tier A (must exist for v1 bootable spine):** boot/handoff, serial, panic path, PMM, allocator, VMM scaffold, traps/interrupts, timer, idle thread, scheduler scaffold, capability-table basics, endpoint IPC basics, self-test harness.
- **Tier B (before claiming microkernel prototype):** user task creation, address-space abstraction, isolated user-space service, driver-boundary stub, syscall/trap contract, basic fault isolation, structured boot event codes.
- **Tier C (explicitly deferred):** GUI/shell, network stack, richer filesystem work, AI governor, advanced personalities, distributed/fabric features, and arm64 runtime support.

See [`docs/architecture/kernel-functionality-tiers-v1.md`](docs/architecture/kernel-functionality-tiers-v1.md) for canonical definitions.

## Execution Plan (Near-term)

### Phase 1 — Stabilize Core

- Harden boot path and early memory initialization checks.
- Add focused kernel self-tests for capability tables and IPC routing.
- Document invariants that verification work depends on.

### Phase 2 — Expand Isolated Services

- Introduce first user-space service reference implementations.
- Add a tiny `hello` service runtime smoke test (launch task, grant one capability, send one IPC, receive one reply, print serial success).
- Define stable interface contracts between kernel and subsystems.
- Add emulator-based integration checks for scheduler + IPC behavior.

### Phase 3 — Platform and Personality Growth

- Extend arm64 bring-up through shared HAL contracts.
- Mature subsystem personalities behind capability boundaries.
- Track deferred features as ADR-backed experiments before merge.

## Build and Emulation

See [`BUILD.md`](BUILD.md) for setup and cross-platform build/emulation workflow.
