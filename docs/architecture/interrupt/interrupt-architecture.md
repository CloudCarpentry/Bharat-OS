---
title: Interrupt Architecture
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - interrupt
see_also:
  - README.md
---
# Interrupt Architecture

## Overview

Bharat-OS employs a **three-layer interrupt architecture** to provide a robust, hardware-agnostic, and feature-rich interrupt handling system. This design decouples the generic OS-level interrupt handling from the specific hardware controller details, allowing for extensibility, clean porting, and support for advanced features like MSI/MSI-X, virtualization, and accelerator profiles.

The architecture strictly separates intent and routing from the hardware mechanisms.

## Three-Layer Architecture

### 1. Core IRQ Layer (Arch-Agnostic Kernel)

The core IRQ layer is the uppermost layer, responsible for managing the logical interrupt lifecycle independent of any hardware.

*   **Virtual IRQ Space (`virq`)**: The kernel primarily deals with virtual IRQ numbers, isolating it from raw hardware lines.
*   **IRQ Descriptors**: Each `virq` maps to a descriptor (e.g., `irq_desc_t` or future `bh_irq_desc_t`). This descriptor holds:
    *   State flags (masked, pending, in-progress, oneshot, level-triggered).
    *   Pointers to the associated `irq_domain` and controller operations.
    *   Handler chains (supporting shared interrupts).
    *   Affinity masks.
*   **Dispatch Flow**: A single, unified dispatch contract is enforced across all architectures:
    `entry -> claim -> domain map -> dispatch -> eoi`
*   **Deferred Work**: Hard IRQ contexts must be brief and deterministic. The core layer manages a queue for deferred work (bottom halves) executed in a pre-emptible context.

*Future Naming Convention*: The core OS primitive type for an interrupt identity will migrate to a canonical `bh_irq_t` to maintain naming consistency across Bharat-OS UAPIs and kernel structures.

### 2. IRQ Domain + Routing Layer

The IRQ domain layer sits between the core OS and the hardware controllers. It is strictly responsible for translation and routing.

*   **Translation**: It translates hardware IRQ numbers (`hwirq`) to virtual IRQ numbers (`virq`), ensuring that drivers do not perform ad-hoc interrupt math.
*   **Hierarchical Domains**: Domains can be stacked. For example, a GPIO expander creates a domain that acts as a child of a GIC SPI domain. An MSI device domain sits above an ITS or IMSIC domain.
*   **MSI/MSI-X Domains**: Message Signaled Interrupts are modeled as domains, providing a generic programming API rather than relying on per-driver, ad-hoc implementations.
*   **Affinity Propagation**: The domain layer propagates affinity requests from the core descriptor down to the hardware controller programming interface.

### 3. Controller Driver Layer (Arch/SoC Specific)

The lowest layer consists of the hardware-specific driver implementations (e.g., APIC/IOAPIC for x86, GICv2/v3+ITS for ARM, PLIC/APLIC/IMSIC for RISC-V).

*   **Controller Ops (`irq_controller_ops_t`)**: Each controller binds to the core layer by providing a standard set of operations: `mask`, `unmask`, `ack`, `eoi`, `set_affinity`, `set_type`, etc.
*   **Feature Declarations**: Controllers expose capabilities (`ARCH_CAP_*`) indicating support for features like hierarchical domains, posted interrupts, direct injection, or MSI remap. This allows the OS to use fast paths opportunistically with guaranteed safe fallbacks.

## Roadmap and Future Enhancements

The implementation of this architecture is executed in planned phases. The target roadmap for the core foundations includes:

1.  **Fully Realizing `irq_domain`**: ✅ **Phase-1 completed** with in-kernel domain allocation and explicit mapping-table based `map -> translate -> unmap` flow. The current implementation uses bounded static tables (`IRQ_DOMAIN_MAX_DOMAINS`, `IRQ_DOMAIN_MAX_MAPPINGS`) to avoid dynamic allocation in hard IRQ paths while preserving deterministic behavior.
2.  **Descriptor State Formalization**: Enriching the `irq_desc` model with explicit state bitfields and active chip pointers.
3.  **Domain-First Routing Enforced**: ✅ **Phase-3 completed**. The generic `hal_interrupt_handle_trap_irq` now mandatorily performs domain translation. Raw hardware interrupts are translated into virtual IRQs before they are dispatched. Also, each primary interrupt controller (APIC for x86_64, GICv3 for ARM, PLIC for RISC-V, etc.) now creates and registers a root domain during HAL boot. Unmapped interrupts fallback safely instead of erroneously dispatching raw hardware identifiers.

*Note: For detailed progression, refer to `ROADMAP.md`.*

## Validation and Bring-up Testing Design

A robust, production-grade test suite is required to validate the interrupt architecture as it is implemented across different profiles.

### 1. Architecture Conformance Tests
These tests validate the required backend controller operations and semantic correctness:
*   **Mask/Unmask Correctness**: Ensuring that masked interrupts do not fire and are appropriately pended.
*   **Level vs. Edge Semantics**: Validating acknowledgment requirements and repetitive firing behavior.
*   **Shared IRQ Behavior**: Testing handler chain execution and unregistration.
*   **Affinity Move Behavior**: Testing dynamic rerouting of interrupts to different CPU cores without loss.
*   **Domain Map/Translate/Unmap Semantics**: Boot selftests now validate domain range checks, duplicate mapping rejection, and translation invalidation after unmap.

### 2. Stress and Load Tests
These tests push the system to identify edge cases, race conditions, and bottlenecks:
*   **Interrupt Storm Handling**: Flooding a line to verify rate-limiting or backpressure mechanisms without hard-locking the core.
*   **Cross-Core IPI Throughput**: Measuring the latency and throughput of inter-processor interrupts under heavy locking contention.
*   **High-Rate MSI-X Queue Simulation**: Simulating multi-queue devices (like NVMe or 100G NICs) to ensure the domain translation and dispatch layers do not introduce unacceptable overhead.

### 3. Regression Matrix
All interrupt logic must pass through the multi-arch headless build and test matrix:
*   `qemu-virt-x86_64` (APIC/IOAPIC paths)
*   `qemu-virt-arm64` (GICv3/ITS paths)
*   `qemu-virt-riscv64` (PLIC/AIA paths)
*   Emulated edge profiles for `arm32` and `riscv32`.

### 4. Observability and Auditing
*   **Tracepoints**: Hooking into `claim`, `dispatch`, and `eoi` to measure handling latency.
*   **Spurious Rate Counters**: Tracking unhandled interrupts.
*   **Health Counters**: Per-controller observability for active/pending metrics.

## Production-Grade Profile and Board Adaptation

To move from architecture-ready to production-grade, interrupt behavior must be selected by a **profile contract** and validated against **board capabilities** at boot.

### Profile-driven interrupt policy

Interrupt runtime behavior should be computed from `profile × arch_caps × board_caps`, not from architecture checks alone.

* **RT/Safety profile**
  * Enforce bounded ISR budget and strict threaded-bottom-half policy.
  * Disallow shared IRQ lines for safety-critical devices unless explicitly waived.
  * Require deterministic affinity pinning and reserved CPU shielding for critical vectors.
* **General-purpose profile**
  * Allow shared IRQs and adaptive interrupt moderation.
  * Prefer throughput-optimized MSI-X spreading across CPUs.
* **Edge/minimal profile (including Harvard-style MCU layouts)**
  * Compile-time trim of advanced routing features (ITS/IMSIC/remapping) when unavailable.
  * Use static vector tables and fixed virq ranges with fail-closed overflow handling.
  * Enforce explicit MMIO-safe controller access wrappers for split instruction/data memory constraints.

### Harvard-architecture readiness requirements

For Harvard or Harvard-like targets (separate instruction/data memory constraints), the interrupt subsystem must additionally guarantee:

1. **Vector/table placement policy**: ISR entry stubs and vector tables are placed in executable memory regions, while mutable descriptor state remains in data memory.
2. **No implicit self-modifying paths**: runtime patching of vector code is prohibited unless profile explicitly enables secure patch flow.
3. **Cache/ordering hooks**: board HAL provides mandatory instruction/data synchronization hooks when changing interrupt routing metadata that affects executable dispatch.
4. **ISR code-size budgeting**: per-profile hard limit for top-half footprint to preserve deterministic instruction memory usage.

### Board/platform feature contract

Each board descriptor should declare an interrupt feature contract used by boot validation:

* Controller topology (single, cascaded, hierarchical domain tree).
* MSI/MSI-X availability and vector count limits.
* Local interrupt controller availability per core/hart.
* Secure/non-secure interrupt partitions (TrustZone-like or equivalent).
* Wakeup-capable interrupt lines for low-power states.

Boot should fail closed (or degrade according to profile policy) when required contract fields are absent.

### New coding tasks for production-grade interrupt stack

1. **Implement `irq_profile_policy` runtime object**
   * Inputs: `arch_caps`, board interrupt descriptor, selected profile.
   * Outputs: allowed IRQ features, ISR budget class, affinity constraints, fallback mode.
2. **Add board interrupt capability schema** (`board_interrupt_caps`)
   * Parse from board/platform descriptors and expose through HAL discovery API.
3. **Introduce Harvard-safe vector placement API**
   * Explicit sections/macros for vector stubs vs mutable IRQ data.
   * Add compile/link assertions for forbidden placement.
4. **Conformance gate: `interrupt_profile_selftest`**
   * Validates profile requirements (e.g., MSI required/forbidden, shared IRQ policy, secure partition policy) during boot tests.
5. **Domain hierarchy validator**
   * Verifies parent/child domain invariants and reports mapping leaks/unbalanced unmap at shutdown test stage.
6. **Interrupt security hardening**
   * Per-IRQ provenance tagging, spoofed-MSI detection hooks, and optional rate-limited quarantine mode.
7. **Board-aware CI matrix expansion**
   * Add at least one board configuration per architecture class (x86 server, arm64 edge, arm32 MCU-like, riscv64 SBC, riscv32 embedded) with interrupt conformance must-pass checks.
