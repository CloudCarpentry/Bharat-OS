# ADR-012: Interrupt Controller Evolution with Compatibility-First Core IRQ Model

- **Status:** Accepted
- **Date:** 2026-03-21
- **Deciders:** Kernel/HAL maintainers
- **Related docs:**
  - `docs/reviews/gap_analysis/interrupt_controller_architecture_plan.md`
  - `docs/architecture/components/kernel-subcomponents-architecture.md`
  - `docs/architecture/arch-capability-matrix.md`

## Context

Bharat-OS currently has usable interrupt controller implementations for x86_64 (APIC family), arm64 (GICv3 direction), and riscv64 (PLIC direction), with early abstraction points for domains and MSI routing.

Historically, the platform mixed two handling styles (`hal_irq_*` and trap-side acknowledge/end sequences) and included architecture-specific `#if defined` logic within common code (`trap.c` and `interrupt_common.c`). This increased bring-up cost for additional targets and made behavior drift likely as more ISA features were introduced.

At the same time, platform goals now explicitly include:

- arm32 support beyond placeholder parity,
- riscv64 hardening while keeping a riscv32 runway,
- compatibility-first evolution that does not break existing x86_64/arm64/riscv64 flows,
- future support for ISA interrupt extensions and accelerator-heavy interrupt profiles.

## Decision

Bharat-OS adopts a **compatibility-first interrupt evolution strategy** with one canonical core IRQ path and strict layering:

1. **Core IRQ layer (kernel-arch agnostic)**
   - owns `irq_desc` lifecycle, handler dispatch, state transitions, accounting, and deferred work.
2. **IRQ domain/routing layer**
   - is the only translation path from hardware interrupt identity to virtual IRQ.
3. **Controller drivers (core/arch/SoC specific)**
   - implement claim/eoi/mask/unmask/type/affinity/ipi ops without embedding policy.

### Compatibility rule (non-breaking requirement)

All migration work must preserve existing behavior by default:

- legacy entry points remain as wrappers until every in-tree backend is switched,
- no architecture loses current boot/interrupt functionality during refactor,
- optional capabilities are runtime-probed and gated,
- every acceleration path has a functionally equivalent fallback.

### Mandatory backend contract (minimum ops)

Every supported interrupt backend must provide:

- `init_boot`
- `init_cpu_local`
- `claim`
- `eoi`
- `mask` / `unmask`
- `set_type` (edge/level)
- `set_affinity`
- `send_ipi` (where architecture supports it)

### Architecture coverage commitments

- **Tier-1 maintained:** x86_64, arm64, riscv64
- **Tier-2 bring-up in this plan:** arm32 (GICv2 baseline first), riscv32 (PLIC baseline)
- **Forward path:** x86 x2APIC/remapping/posted-interrupts, ARM GICv4+ options, RISC-V AIA (APLIC/IMSIC)

### Extensibility requirement

Interrupt-related ISA extensions and accelerators (NPU/GPU/DSP/PCIe MSI-X heavy devices) are integrated through capability bits and domain composition only, not by bypassing irq-core.

## Consequences

### Positive

- Reduces architecture-specific trap divergence.
- Gives a clear non-breaking migration path for existing backends.
- Makes arm32/riscv32 additions incremental rather than invasive.
- Keeps future ISA/accelerator enhancements additive.

### Costs / trade-offs

- Requires staged conversion with temporary adapter glue. *(Completed: core trap flows successfully unified).*
- Requires stronger conformance testing matrix across five architectures.
- Adds documentation and contract discipline around capability probing.

### Follow-up actions

1. Update kernel subcomponent architecture docs with explicit interrupt submodule status and phased non-breaking migration order.
2. Keep gap-analysis plan synchronized with the compatibility-first rules and architecture parity matrix.
3. Track backend conformance against the mandatory ops contract in implementation reviews.
