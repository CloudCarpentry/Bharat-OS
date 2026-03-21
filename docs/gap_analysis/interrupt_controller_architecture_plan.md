# Interrupt Controller Architecture Analysis & Evolution Plan

## Scope and target architectures

This plan defines how Bharat-OS can evolve one interrupt subsystem that supports:

- x86_64 (APIC/IOAPIC today, x2APIC/posted-interrupt features later)
- arm64 (GICv3 + ITS)
- arm32 (GICv2/v3-compatible bring-up path)
- riscv32 and riscv64 (PLIC today, AIA/APLIC/IMSIC path forward)
- future SoCs/boards without reworking core kernel interrupt logic

## Current architecture snapshot (what we have now)

### Strengths

1. **A generic IRQ API exists** (`hal_irq_*`, `hal_interrupt_*`) with registration, shared handlers, affinity metadata, and deferred work hooks.
2. **Per-arch controller backends exist** for:
   - x86_64 APIC (`kernel/src/hal/x86_64/apic.c`)
   - arm64 GICv3 (`kernel/src/hal/arm64/gicv3.c`)
   - riscv64 PLIC (`kernel/src/hal/riscv64/plic.c`)
3. **Discovery already models multiple interrupt controllers** (APIC, IOAPIC, GICv2/v3, ITS, PLIC, AIA placeholders) in `hal_discovery.h`.
4. **MSI-domain concept exists** (`device/irq_domain.h`) and is partially wired on arm64 ITS + x86 stubs.

### Gaps / architectural risks

1. **Two interrupt abstraction families are mixed**:
   - `hal_irq_*` + `hal_interrupt_dispatch()` path
   - `hal_interrupt_acknowledge()/end_of_interrupt()` path in trap handling
   This creates inconsistent behavior and makes cross-arch features harder.
2. **Controller ops are not actually stored/applied**: `hal_irq_set_controller()` currently returns success but does not bind ops into descriptors.
3. **Trap path is not uniformly controller-agnostic**: architecture-specific branches directly special-case timer/ack flow.
4. **Affinity and routing are metadata-first, hardware-second**: generic affinity is tracked, but backend reprogramming is mostly stubbed.
5. **arm32/riscv32 parity is incomplete** for a production interrupt stack.
6. **Feature-level extensibility (ISR budget, QoS, NMI/FIQ policy, virtualization interrupt injection) is not first-class yet.**

## Target architecture model

Adopt a **three-layer interrupt architecture**:

1. **Core IRQ layer (arch-agnostic kernel)**
   - IRQ descriptors, handler chains, state machine, accounting, deferred work, and policy.
2. **IRQ domain + routing layer**
   - Maps hardware interrupt identities to kernel virq space.
   - Composes hierarchical domains (e.g., GPIO expander -> GIC SPI; MSI device -> ITS/IMSIC).
3. **Controller driver layer (arch/SoC specific)**
   - APIC/IOAPIC, GICv2/v3+ITS, PLIC/APLIC/IMSIC, board-local intc.

### Core design principles

- **Single dispatch contract** for all architectures: `entry -> claim -> domain map -> dispatch -> eoi`.
- **Hierarchical irq_domain as the only translation mechanism** (no ad-hoc IRQ math in drivers).
- **Hard IRQ fast path bounded and deterministic**; heavy work is deferred.
- **Per-CPU interrupt context explicit** (no hardcoded CPU 0 assumptions).
- **Capabilities declare optional accelerations**, but fall back safely.

## Proposed implementation plan

## Non-breaking implementation set (safe to do first)

The following items can be implemented with minimal regression risk because they preserve existing backend behavior and mostly tighten internal contracts:

1. [x] **Controller binding correctness**: make `hal_irq_set_controller()` persist and validate ops in descriptors while keeping existing backend entry points unchanged.
2. [x] **Trap-flow adapters**: add a unified internal helper (`claim -> translate -> dispatch -> eoi`) and call it from current arch trap paths before removing any old glue. *(Completed: unified via `hal_interrupt_handle_trap_irq` and `hal_interrupt_get_active_irq` across all archs)*.
3. [x] **Descriptor state formalization**: add `irq_desc` state bits and counters without changing externally visible IRQ numbers or driver APIs.
4. [ ] **Domain-first for new routes only**: require `irq_domain` for newly added devices/routes, while legacy static routes continue to work during migration.
5. [x] **Affinity propagation no-op fallback**: wire descriptor-to-backend affinity propagation with backend fallback to current behavior when hardware reprogramming is not yet implemented.
6. [ ] **Capability-gated fast paths**: introduce capability probes for x2APIC/GIC advanced modes/AIA hooks but keep fast paths disabled by default unless fully validated.

These steps align with **ADR-012 compatibility-first rules** and should be completed before any destructive API removal.

## Phase 1: Unify core interrupt flow (foundation) -> **COMPLETED**

1. [x] Create a canonical per-CPU trap/IRQ flow:
   - `controller->claim()` returns hwirq/token
   - `irq_domain_translate()` gives virq
   - `irq_core_dispatch(virq)` executes handlers
   - `controller->eoi(token)` finalizes
2. [x] Remove duplicated trap-path variants by introducing one internal API used by x86/arm/riscv entries. *(Completed: `trap.c` fully deduplicated, relying on `hal_interrupt_get_active_irq` for all architectures).*
3. [x] Upgrade `irq_desc` to include:
   - bound controller ops pointer
   - bound domain pointer
   - irq state bits (masked, pending, in-progress, oneshot, level)
4. [x] Make `hal_irq_set_controller()` actually bind and validate ops.

## Phase 2: Domain-first routing and MSI/MSI-X

1. Make `device/irq_domain` the required path for all external IRQ routing.
2. Implement parent/child domains:
   - x86: IOAPIC domain + MSI/MSI-X domain
   - arm64: GIC SPI/PPI + ITS LPI domain
   - riscv: PLIC domain now; APLIC/IMSIC domains later
3. Introduce generic MSI message programming API at domain layer (not per-driver ad-hoc).
4. Add irq affinity propagation from core descriptor -> domain -> controller programming.

## Phase 3: Architecture parity matrix (x86_64, arm32, arm64, riscv32, riscv64)

1. Define mandatory backend ops for all target arches:
   - init_boot, init_cpu_local, mask/unmask, set_type(edge/level), set_affinity, claim, eoi, send_ipi
2. Build explicit support tracks:
   - **x86_64**: LAPIC/IOAPIC complete path + MSI/MSI-X robust path + x2APIC optional.
   - **arm64**: GICv3 distributor/redistributor correctness + ITS LPI lifecycle.
   - **arm32**: GICv2 baseline first (SPIs/PPIs/SGIs), then optional GICv3 compatibility mode.
   - **riscv64/riscv32**: PLIC baseline + separate local interrupt handling + AIA roadmap.
3. Add `ARCH_CAP_*` capability bits for interrupt features:
   - hierarchical domains, MSI remap, posted interrupts, direct injection, irq-priority levels.

## Phase 4: Future hardware and accelerator-ready design

1. Accelerator IRQ profile support (NPU/GPU/DSP/PCIe accelerators):
   - multi-queue MSI-X vector allocation policies
   - per-device interrupt moderation knobs
   - isolated IRQ delivery for secure domains
2. Virtualization-ready hooks:
   - vIRQ injection API (for future VMM path)
   - interrupt remapping/IOMMU integration points
3. Real-time profile support:
   - bounded ISR runtime accounting
   - priority classing and CPU shielding

## Hardware ISA extensions and acceleration strategy

Use ISA/platform extensions opportunistically through capability probing:

- **x86_64**: x2APIC, interrupt remapping, posted interrupts, TSC-deadline timer.
- **ARM**: GICv4/v4.1 direct injection when available, LPIs for high-queue devices.
- **RISC-V**: AIA (APLIC+IMSIC), CLIC-like fine-grained local interrupt features (platform dependent).

Rules:

1. Never make extensions mandatory for correctness.
2. Keep fallback path functionally equivalent.
3. Gate all fast paths behind capability bits + runtime probe.

## Recommended data model changes

1. Add `irq_chip_instance` objects (controller instance with MMIO base, cpu affinity, parent domain).
2. Add `irq_desc.state` bitfield and `irq_desc.chip` pointer.
3. Replace fixed `HAL_MAX_IRQS=256` assumption with configurable virq allocator.
4. Add per-CPU `irq_ctx` with stats, nesting depth, deferred queue, and preemption accounting.

## Validation and bring-up plan

1. **Architecture conformance tests**:
   - mask/unmask correctness
   - level vs edge semantics
   - shared IRQ behavior
   - affinity move behavior
2. **Stress tests**:
   - interrupt storm handling
   - cross-core IPI throughput
   - high-rate MSI-X queue simulation
3. **Regression matrix**:
   - qemu-virt-x86_64
   - qemu-virt-arm64
   - qemu-virt-riscv64
   - emulated/board profile for arm32 and riscv32 where available
4. **Observability**:
   - tracepoints for claim/dispatch/eoi latency and spurious IRQ rate
   - per-controller health counters

## Practical execution order (recommended)

1. ~~**Week 1-2**: Unify trap/IRQ flow and wire real controller ops binding.~~ **(Completed)**
2. **Week 3-4**: Domain-first conversion for existing x86_64/arm64/riscv64 backends.
3. **Week 5-6**: arm32 + riscv32 baseline backend completion.
4. **Week 7+**: MSI-X scaling, accelerator policies, AIA/x2APIC advanced features.

## ADR alignment

This plan is governed by `docs/decisions/ADR-012-interrupt-controller-evolution.md`, especially:

- staged migration with compatibility wrappers,
- tiered architecture commitments (x86_64/arm64/riscv64 maintained, arm32+riscv32 bring-up),
- runtime-gated ISA extension and accelerator support with guaranteed fallback.

## Decision summary

To support current and future hardware cleanly, Bharat-OS should move from “controller-specific IRQ handling in many places” to a **single irq-core + hierarchical domain + pluggable chip instances** model. This gives:

- portability across x86_64, arm32, arm64, riscv32, riscv64
- lower bring-up cost for new SoCs
- cleaner path to ISA extensions and accelerator-heavy platforms
- stronger real-time and virtualization readiness
