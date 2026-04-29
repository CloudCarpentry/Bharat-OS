# Kernel Primitive Contract

## Status
Transitional / Accepted

## Scope
This contract defines the core mechanism boundaries of the Bharat-OS kernel. It identifies what belongs in the kernel as a "primitive" and what must remain outside as policy or higher-level services.

## Non-Goals
- Defining specific scheduling algorithms (Policy).
- Managing AI model routing.
- Device-specific configuration decisions.

## Ownership Boundary
- **Kernel:** Owns core mechanisms (context switching, page table management, capability lookups, uRPC transport).
- **HAL:** Provides raw hardware discovery and access abstractions.
- **Services:** Implement domain-specific policies (e.g., process lifecycle, file systems, network stacks).

## Contract
The kernel exposes a set of primitive classes defined in `bh_kernel_primitive_class_t`. Availability and support levels can be queried at runtime.

### Primitive Classes
- `BH_PRIMITIVE_SCHED`: CPU scheduling and thread management.
- `BH_PRIMITIVE_MEMORY`: Virtual and physical memory management.
- `BH_PRIMITIVE_CAPABILITY`: Object-based access control.
- `BH_PRIMITIVE_IPC`: Inter-process communication.
- `BH_PRIMITIVE_TIMER`: Timekeeping and alarms.
- `BH_PRIMITIVE_FAULT`: Exception and trap handling.
- `BH_PRIMITIVE_DMA`: Direct memory access management.
- `BH_PRIMITIVE_ACCEL`: Hardware accelerator support.
- `BH_PRIMITIVE_TELEMETRY`: Kernel-level tracing and monitoring.

## Hardware Support Levels
- `BH_PRIMITIVE_UNSUPPORTED`: Not available on this platform.
- `BH_PRIMITIVE_STUBBED`: API exists but does nothing (no-op).
- `BH_PRIMITIVE_SOFTWARE_FALLBACK`: Functional via software implementation.
- `BH_PRIMITIVE_HARDWARE_ASSISTED`: Functional with some hardware acceleration.
- `BH_PRIMITIVE_HARDWARE_ENFORCED`: Fully backed and enforced by hardware (e.g., MMU, IOMMU).

## Failure Behavior
If a primitive is requested but unsupported, the kernel must return `K_ERR_UNSUPPORTED`. If hardware enforcement is required by policy but absent, operations must fail closed.

## Security Invariants
- No policy-heavy logic in the kernel.
- Primitives must be capability-backed where they cross isolation boundaries.
- Support levels must accurately reflect the underlying hardware enforcement.

## Testing Requirements
- Host-based unit tests for the primitive registry.
- QEMU-based boot tests to verify hardware discovery and summary printing.
- Negative tests for unsupported primitives.
