# Architecture Documentation Index

This folder captures the current architectural baseline for Bharat-OS.

## How to read this section

1. Start with platform bring-up flows.
2. Continue with kernel security and object/IPC/memory models.
3. Review deferred areas (GUI, personalities) with their status labels.
4. Cross-check major architectural constraints against ADRs in `docs/adr/`.

## Recommended reading order

### Core Architecture

- [Profile-Driven Multi-Kernel Architecture](core/profile-driven-multi-kernel-arc.md)
- [Low-Footprint Design and Analysis](low-footprint-design.md)

### Boot and bring-up

- [`v1-boot-definition.md`](v1-boot-definition.md)
- [`kernel-functionality-tiers-v1.md`](kernel-functionality-tiers-v1.md)
- [`boot/boot_flow_x86_64.md`](boot/boot_flow_x86_64.md)
- [`boot/boot_flow_riscv64.md`](boot/boot_flow_riscv64.md)
- [`boot/boot_display_architecture.md`](boot/boot_display_architecture.md)
- [`boot/boot_console_uart_gui_brainstorm.md`](boot/boot_console_uart_gui_brainstorm.md)
- [`SHAKTI_BHARAT_OS_IMPLEMENTATION_PLAN.md`](SHAKTI_BHARAT_OS_IMPLEMENTATION_PLAN.md)

### Core kernel architecture

- [`kernel-api.md`](kernel-api.md)
- [`capability-model.md`](capability-model.md)
- [`kernel-object-model.md`](kernel-object-model.md)
- [`ipc-model.md`](ipc-model.md)
- [`capability-and-ipc-baseline.md`](capability-and-ipc-baseline.md)
- [`memory/memory-model.md`](memory/memory-model.md)
- [`multiprocessor-and-numa-baseline.md`](multiprocessor-and-numa-baseline.md)
- [`scheduler-and-threading.md`](scheduler-and-threading.md)
- [`realtime-automotive-robotics-subsystems.md`](realtime-automotive-robotics-subsystems.md)
- [`ai-scheduler-status-and-roadmap.md`](ai-scheduler-status-and-roadmap.md)
- [`syscall-trap-gates.md`](syscall-trap-gates.md)
- [`driver-model.md`](driver-model.md)
- [`hardware-abstraction-and-drivers-baseline.md`](hardware-abstraction-and-drivers-baseline.md)
- [`storage-and-sandbox-strategy.md`](storage-and-sandbox-strategy.md)
- [`verification-scope.md`](verification-scope.md)
- [`threat-model-and-mac.md`](threat-model-and-mac.md)
- [`cmake-governance-and-agent-rules.md`](cmake-governance-and-agent-rules.md)


### Component-level deep dives (with Mermaid + PlantUML)

- [`components/kernel-subcomponents-architecture.md`](components/kernel-subcomponents-architecture.md)
- [`components/subsystem-subcomponents-architecture.md`](components/subsystem-subcomponents-architecture.md)
- [`components/services-subcomponents-architecture.md`](components/services-subcomponents-architecture.md)
- [`components/drivers-subcomponents-architecture.md`](components/drivers-subcomponents-architecture.md)

### Deployment and user-space strategy

- [`device-profiles-and-use-cases.md`](device-profiles-and-use-cases.md)
- [`distributed-kernel-implementation-plan.md`](distributed-kernel-implementation-plan.md)

### System shell architecture

- [`system/shell-architecture.md`](system/shell-architecture.md)
- [`system/shell-profile-matrix.md`](system/shell-profile-matrix.md)

### User-space strategy and deferred surfaces

- [`personality-layer.md`](personality-layer.md)
- [`gui-strategy.md`](gui-strategy.md)
- [`device-profiles.md`](device-profiles.md)
- [`ai-scheduler-overview.md`](ai-scheduler-overview.md)

## Scope note

Architecture docs describe both:

- **In-scope v1 core** (bootable, verifiable kernel spine), and
- **Deferred/experimental directions** (documented for design continuity).

The ADR set is the source of truth whenever scope or priority conflicts arise.

## Related ADR updates

- [`docs/adr/ADR-008-ai-scheduler-plugin-contract.md`](../adr/ADR-008-ai-scheduler-plugin-contract.md)
- [`docs/adr/ADR-009-documentation-status-and-claims.md`](../adr/ADR-009-documentation-status-and-claims.md)
