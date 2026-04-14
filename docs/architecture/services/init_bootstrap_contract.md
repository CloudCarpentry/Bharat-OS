# Init Bootstrap Contract

This document outlines the architecture and boundaries of the `init` service (`services/core/init`) within the Bharat-OS multikernel ecosystem.

## Objective

`init` is the first userspace bootstrap coordinator. It is designed to be a small, deterministic, profile-aware bootstrap service that starts the minimal valid runtime graph. It avoids becoming a full service supervisor, policy engine, or process/VM manager, deferring those responsibilities to specialized managers (e.g., `servicemgr`, `process_manager`, `vm_manager`).

## Responsibility Boundary

### What `init` Owns
- **Boot Profile Selection:** Determines the active execution profile (e.g., Tiny, Desktop).
- **Manifest Filtering:** Filters the registered service manifest by profile, capability mask, and board/personality mask.
- **Startup Sequencing:** Deterministically resolves dependencies and starts services in the correct order.
- **Bounded Retry Behavior:** Provides minimal restart loops capped by the static manifest.
- **Startup Result Reporting:** Outputs simple status logs or state exports.
- **Supervisor Handoff:** Provides a clean handover boundary to `servicemgr` (if present) for long-term supervision.

### What `init` Does Not Own
- Long-term crash supervision or rich restart policies.
- Process or address-space lifecycle semantics.
- Storage, telemetry, crypto, or device driver logic.
- Dynamic dependency loading (e.g., JSON/filesystem config parsing).

## Profile Matrix

`init` supports canonical profiles defined in `bharat_init_profile_t`, including:
- **TINY (`BHARAT_INIT_PROFILE_TINY`):** Extremely constrained builds. No dynamic loading, minimal or zero graph, no restart loops.
- **SMALL (`BHARAT_INIT_PROFILE_SMALL`):** Service-capable embedded devices. Includes `namesvc`, optional domain services.
- **EMBEDDED_RICH (`BHARAT_INIT_PROFILE_EMBEDDED_RICH`):** Includes `process_manager`, `vm_manager`, and supervisor targets.
- **MOBILE / DESKTOP / DRONE:** Specialized feature combinations (e.g., display services, diagnostics, safe-mode bias).

## Boot Context Model

The `init_boot_context_t` object captures the system environment:
- `profile`: Selected init profile.
- `arch_id`, `platform_id`, `board_id`, `personality_id`: System descriptors.
- `cap_mask`: Available capability mask (e.g., network, mmu).
- `safe_mode`: Flag indicating fallback/safe boot path.

## Manifest Format

The service selection is driven by a static compile-time array (`g_init_manifest`) of `init_service_desc_t` objects. It is strictly bounded and heap-free in its core parsing logic.

## Required vs Optional Semantics

- **Required Services (`INIT_SERVICE_REQUIRED`):** If a required service fails to start after exhausting its retry limit, `init` will mark the boot context as `safe_mode` and fail the boot sequence.
- **Optional Services (`INIT_SERVICE_OPTIONAL`):** Failure does not fail the boot sequence.

## Tiny vs Rich Mode

For tiny profiles, `init` compiles out advanced logic, reduces the retry limits, and starts a minimal or single payload, operating as a zero-heap static forwarder.

## Handoff Boundary

Once the bootstrap graph settles, `init` can hand off control to a richer supervisor (e.g., `servicemgr`) via the `init_handoff_to_supervisor` hook. If the supervisor is absent, `init` idles and retains status.
