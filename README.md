# Bharat-OS

<p align="center">
  <img src="assets/branding/logo-icon.png" alt="Bharat-OS logo" width="140" />
</p>

<p align="center">
  <img src="assets/branding/banner-dark.png" alt="Bharat-OS banner" />
</p>

<p align="center"><em>Official Bharat-OS logo and banner assets</em></p>

---

Bharat-OS is a capability-oriented microkernel project with a multikernel direction. The repository currently delivers a **bootable and testable kernel baseline** plus architecture documentation for deferred and experimental tracks.

## Project status at a glance

| Area | Current status | Notes |
| --- | --- | --- |
| Capability model and IPC baseline | Implemented baseline | Capability tables, delegated rights checks, endpoint IPC + URPC scaffolding. |
| Memory management | Partial baseline | PMM/buddy allocator and VMM mapping registry exist; full hardware page-table manager remains deferred. |
| Scheduler and AI hook points | Implemented baseline | Timer-driven scheduler path with bounded AI suggestion ingestion/processing; full SMP runqueues/context switching remain deferred. |
| Driver and HAL model | Implemented baseline | HAL contracts and driver framework scaffolding exist across x86_64, riscv64, arm64. |
| Distributed/multikernel scale-out | Early baseline | Per-core URPC matrix and multicore bootstrap hooks exist; production-grade topology and transport tuning are deferred. |

For architecture-level details and deferred boundaries, see `docs/architecture/` and ADRs in `docs/decisions/`.

## Device Profiles & Use-cases

Bharat-OS targets multiple deployment classes. These profiles describe **how the current baseline maps to real devices today**, and what is planned next:

- **Mobile / Wearables (EDGE profile):** capability isolation, bounded footprint, and power-aware scheduling hooks are available now; production-grade power control policy is roadmap.
- **Robotics / Drones (EDGE + RTOS-leaning):** deterministic IPC pathways and architecture portability are present; stronger real-time admission and fault-containment depth are roadmap.
- **Network appliances / Edge gateways:** capability-mediated driver boundaries and multikernel messaging baseline are present; mature data-plane acceleration is roadmap.
- **Data-center / clustered nodes:** NUMA/multicore scaffolding and URPC primitives are present; full distributed scheduling and high-scale service orchestration are roadmap.

Detailed mapping is documented in [`docs/architecture/device-profiles-and-use-cases.md`](docs/architecture/device-profiles-and-use-cases.md).

## AI Features & Roadmap

Bharat-OS keeps AI policy outside ring-0 while exposing bounded kernel mechanisms:

### Implemented baseline

- Kernel-side telemetry collection hooks and bounded AI suggestion queueing.
- Scheduler action handling for migrate/priority/throttle suggestion types.
- Capability-guarded governor control-plane endpoint.
- Architecture/profile-neutral telemetry plugin contract (with fallback behavior when PMCs are unavailable).

### Roadmap

- Better telemetry quality (hardware PMC integrations per architecture).
- Per-core runqueues + richer migration policy under SMP load.
- Safety/verification hardening for AI-driven scheduling decisions.
- Clearer user-space governor lifecycle, observability, and audit reporting.

See [`docs/architecture/ai-scheduler-status-and-roadmap.md`](docs/architecture/ai-scheduler-status-and-roadmap.md) and [`docs/decisions/ADR-008-ai-scheduler-plugin-contract.md`](docs/decisions/ADR-008-ai-scheduler-plugin-contract.md).

## Core architecture themes

- **Capability-based security:** object rights, delegation constraints, and explicit authority checks.
- **Microkernel layering:** small kernel core with user-space policy and service growth path.
- **Multikernel direction:** explicit messaging-oriented coordination across cores and eventually nodes.
- **Profile-aware composition:** RTOS/EDGE/DESKTOP profile tuning with bounded kernel mechanisms.

## Build quick start

### Prerequisites

- `cmake` (3.20+)
- A supported cross toolchain such as:
  - `riscv64-unknown-elf-*`
  - `aarch64-none-elf-*`
  - `x86_64-elf-*`

### Build examples

```bash
# RISC-V
./tools/build.sh riscv64

# ARM64
./tools/build.sh arm64

# x86_64 (optionally run in QEMU)
./tools/build.sh x86_64 --run
```

Windows users can use `tools/build.ps1`.

## Repository layout

- `kernel/` — microkernel core (MM, IPC, scheduler, HAL, capability system).
- `subsys/` — subsystem services (including AI governor bridge layer).
- `lib/` — shared user-space facing library surfaces.
- `tests/` — host-based tests for kernel/runtime components.
- `docs/` — architecture docs, ADRs, and implementation reviews.

## Research references

This project aligns with established systems research and uses those works as design guidance:

- Barrelfish multikernel model (messaging-first multicore OS design).
- seL4 capability model and verification-oriented discipline.
- L4-family microkernel separation and minimal trusted core concepts.
- AI-assisted resource management literature (policy guidance in user space, bounded kernel enforcement).

These references are informational guidance for architecture direction, not claims of feature parity.
